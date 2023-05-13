#include "Particle.h"

#if HAL_PLATFORM_NRF52840 

#include "Microphone_PDM.h"

Microphone_PDM_nRF52::Microphone_PDM_nRF52() {

}

Microphone_PDM_nRF52::~Microphone_PDM_nRF52() {

}


int Microphone_PDM_nRF52::init() {
	Hal_Pin_Info *pinMap = HAL_Pin_Map();

	pinMode(clkPin, OUTPUT);
	pinMode(datPin, INPUT);

	attachInterruptDirect(PDM_IRQn, nrfx_pdm_irq_handler, false);

	uint8_t nrfClkPin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[clkPin].gpio_port, pinMap[clkPin].gpio_pin);
	uint8_t nrfDatPin = (uint8_t)NRF_GPIO_PIN_MAP(pinMap[datPin].gpio_port, pinMap[datPin].gpio_pin);

	// Start with default vales
	nrfx_pdm_config_t config = NRFX_PDM_DEFAULT_CONFIG(nrfClkPin, nrfDatPin);

	// Override with everything we have local copies of
	config.mode = stereoMode ? NRF_PDM_MODE_STEREO : NRF_PDM_MODE_MONO;
	config.clock_freq = freq;
	config.edge = edge;
	config.gain_l = gainL;
	config.gain_r = gainR;

	// Initialize!
	nrfx_err_t err = nrfx_pdm_init(&config, dataHandlerStatic);

	return (int)err;
}

int Microphone_PDM_nRF52::uninit() {
	availableSamples = NULL;

	nrfx_pdm_uninit();

	pinMode(clkPin, INPUT);

    return 0;
}

int Microphone_PDM_nRF52::start() {
	useBufferA = true;
	availableSamples = NULL;

	nrfx_err_t err = nrfx_pdm_start();

	return (int)err;
}

int Microphone_PDM_nRF52::stop() {
	availableSamples = NULL;

	nrfx_err_t err = nrfx_pdm_stop();

	return (int)err;
}


void Microphone_PDM_nRF52::dataHandler(nrfx_pdm_evt_t const * const pEvent) {
	/*
 	bool             buffer_requested;  ///< Buffer request flag.
    int16_t *        buffer_released;   ///< Pointer to the released buffer. Can be NULL.
    nrfx_pdm_error_t error;             ///< Error type.
	 */

	if (pEvent->buffer_released) {
		// Adjust samples here
		int16_t *src = (int16_t *)pEvent->buffer_released;

		if (outputSize == OutputSize::UNSIGNED_8) {
			uint8_t *dst = (uint8_t *)src;

			// Scale the 16-bit signed values to an appropriate range for unsigned 8-bit values
			int16_t div = (int16_t)(1 << (size_t) range);

			for(size_t ii = 0; ii < BUFFER_SIZE_SAMPLES; ii++) {
				int16_t val = src[ii] / div;

				// Clip to signed 8-bit
				if (val < -128) {
					val = -128;
				}
				if (val > 127) {
					val = 127;
				}

				// Add 128 to make unsigned 8-bit (offset)
				dst[ii] = (uint8_t) (val + 128);
			}

		}
		else
		if (outputSize == OutputSize::SIGNED_16) {
			int32_t mult = (int32_t)(1 << (8 - (size_t) range));
			for(size_t ii = 0; ii < BUFFER_SIZE_SAMPLES; ii++) {
				// Scale to signed 16 bit range
				int32_t val = (int32_t)src[ii] * mult;

				// Clip to signed 16-bit
				if (val < -32767) {
					val = -32767;
				}
				if (val > 32768) {
					val = 32868;
				}

				src[ii] = (int16_t) val;
			}
		}
		availableSamples = src;

		if (interruptCallback) {
			interruptCallback(src, BUFFER_SIZE_SAMPLES);
		}
	}

	if (pEvent->buffer_requested) {
		if (useBufferA) {
			nrfx_pdm_buffer_set(samples, BUFFER_SIZE_SAMPLES);
		}
		else {
			nrfx_pdm_buffer_set(&samples[BUFFER_SIZE_SAMPLES], BUFFER_SIZE_SAMPLES);
		}
		useBufferA = !useBufferA;
	}
}



// [static]
void Microphone_PDM_nRF52::dataHandlerStatic(nrfx_pdm_evt_t const * const pEvent) {
	Microphone_PDM::instance().dataHandler(pEvent);
}


#endif // HAL_PLATFORM_NRF52840
