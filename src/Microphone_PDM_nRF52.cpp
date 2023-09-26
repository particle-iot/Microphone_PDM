#include "Particle.h"

#if HAL_PLATFORM_NRF52840 

#include "Microphone_PDM.h"

Microphone_PDM_nRF52::Microphone_PDM_nRF52() : Microphone_PDM_Base(BUFFER_SIZE_SAMPLES) {

}

Microphone_PDM_nRF52::~Microphone_PDM_nRF52() {

}


int Microphone_PDM_nRF52::init() {
#if SYSTEM_VERSION >= SYSTEM_VERSION_DEFAULT(5, 0, 0)
	Hal_Pin_Info *pinMap = hal_pin_map();
#else
	Hal_Pin_Info *pinMap = HAL_Pin_Map();
#endif

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
	currentSampleAvailable = NULL;

	nrfx_pdm_uninit();

	pinMode(clkPin, INPUT);

    return 0;
}

int Microphone_PDM_nRF52::start() {
	useBufferA = true;
	currentSampleAvailable = NULL;

	nrfx_err_t err = nrfx_pdm_start();

	return (int)err;
}

int Microphone_PDM_nRF52::stop() {
	currentSampleAvailable = NULL;

	nrfx_err_t err = nrfx_pdm_stop();

	return (int)err;
}


bool Microphone_PDM_nRF52::samplesAvailable() const {
	return (currentSampleAvailable != NULL);
}

bool Microphone_PDM_nRF52::copySamples(void*pSamples) {
	if (currentSampleAvailable) {
		copySamplesInternal(currentSampleAvailable, (uint8_t *)pSamples);
		currentSampleAvailable = NULL;
		return true;
	}
	else {
		return false;
	}
}

bool Microphone_PDM_nRF52::noCopySamples(std::function<void(void *pSamples, size_t numSamples)>callback) {
	if (currentSampleAvailable) {
		copySamplesInternal(currentSampleAvailable, (uint8_t *)currentSampleAvailable);
		callback(currentSampleAvailable, getNumberOfSamples());
		currentSampleAvailable = NULL;
		return true;
	}
	else {
		return false;
	}

}

size_t Microphone_PDM_nRF52::copySrcIncrement() const {
	if (sampleRate == 8000) {
		return 2;
	}
	else {
		return 1;
	}
}



void Microphone_PDM_nRF52::dataHandler(nrfx_pdm_evt_t const * const pEvent) {
	/*
 	bool             buffer_requested;  ///< Buffer request flag.
    int16_t *        buffer_released;   ///< Pointer to the released buffer. Can be NULL.
    nrfx_pdm_error_t error;             ///< Error type.
	 */

	if (pEvent->buffer_released) {
		currentSampleAvailable = (int16_t *)pEvent->buffer_released;;
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
