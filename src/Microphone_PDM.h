#ifndef __Microphone_PDM_H
#define __Microphone_PDM_H

#include "Particle.h"

#if defined(HAL_PLATFORM_RTL872X) && HAL_PLATFORM_RTL872X
	#include "Microphone_PDM_RTL872x.h"
#elif HAL_PLATFORM_NRF52840 
	#include "Microphone_PDM_nRF52.h"
#else
	#error "unsupported platform"
#endif


/**
 * @brief Class for using the hardware PDM (pulse-density modulation) audio decoder on RTL872x and nRF52 devices
 *
 * This works on P2 and Photon 2 (RTL8721DM) as well as Boron, B Series SoM, Tracker SoM, and Argon (nRF52840).
 * 
 */
class Microphone_PDM : public Microphone_PDM_MCU {
public:
	/**
	 * @brief Get the singleton instance of this class
	 * 
	 * @return Microphone_PDM& 
	 */
	static Microphone_PDM &instance();

	/**
	 * @brief Configuration for the size of the output data
	 */
	enum class OutputSize {
		UNSIGNED_8,	 	//!< Output unsigned 8-bit values (adjusted by PDMRange)
		SIGNED_16,		//!< Output signed 16-bit values (adjusted by PDMRange) (default)
		RAW_SIGNED_16	//!< Output values as signed 16-bit values as returned by MCU (unadjusted)
	};

	/**
	 * @brief Adjustment value so samples fill most of an 8 or 16 bit output size
	 * 
	 * The setting is dependent on your microphone. The Adafruit microphone is 12-bit, so the default
	 * value of RANGE_2048 is correct.
	 */
	enum class Range {
		RANGE_128 = 0, 	//!< From -128 to 127 (8 bits)
		RANGE_256, 		//!< From -256 to 255 (9 bits)
		RANGE_512, 		//!< From -512 to 511 (10 bits)
		RANGE_1024, 	//!< From -1024 to 1023 (11 bits)
		RANGE_2048, 	//!< From -2048 to 2047 (12 bits) (default)
		RANGE_4096, 	//!< From -4096 to 4095 (13 bits)
		RANGE_8192, 	//!< From -8192 to 8191 (14 bits)
		RANGE_16384, 	//!< From -16384 to 16383 (15 bits)
		RANGE_32768, 	//!< From -32768 to 32767 (16 bits) (same as raw mode)
	};



	/**
	 * @brief Initialize the PDM module.
	 *
	 * This is often done from setup(). You can defer it until you're ready to sample if desired,
	 * calling right before start().
	 */
	int init() {
		return Microphone_PDM_MCU::init();
	}

	/**
	 * @brief Uninitialize the PDM module. Not supported on RTL872x (P2, Photon 2)!
	 *
	 * Releases the resources used by the PDM module, though the 2K RAM buffer is statically 
	 * allocated on the heap and is not freed.
	 */
	int uninit() {
		return Microphone_PDM_MCU::uninit();
	}

	/**
	 * @brief Start sampling
	 */
	int start() {
		return Microphone_PDM_MCU::start();
	}

	/**
	 * @brief Stop sampling
	 * 
	 * On the RTL872x (P2, Photon 2), it's not actually possible to stop sampling once you
	 * start it, but calling stop will prevent the buffer from being processed. Buffer 
	 * notifications will stop, 
	 */
	int stop() {
		return Microphone_PDM_MCU::stop();
	}

	/**
	 * @brief Return true if there is data available to be copied using copySamples
	 * 
	 * @return true 
	 * @return false 
	 */
	bool samplesAvailable() const {
		return Microphone_PDM_MCU::samplesAvailable();
	}

	/**
	 * @brief Return the number of int16_t samples that copySamples will copy
	 * 
	 * @return size_t Number of uint16_t samples. Number of bytes is twice that value
	 * 
	 * You will never get a partial buffer of data. The number of samples is a constant
	 * that is determined by the MCU type at compile time and does not change. 
	 * 
	 * On the nRF52, it's 512 samples (1024 bytes).
	 * 
	 * On the RTL872x, it's 256 samples (512 bytes). It's smaller because the are 4 buffers instead of the
	 * 2 buffers used on the nRF52, and the optimal DMA size on the RTL872x is 512 bytes.
	 */
	size_t getNumberOfSamples() const {
		return Microphone_PDM_MCU::BUFFER_SIZE_SAMPLES;
	}

	/**
	 * @brief Copy samples from the DMA buffer to your buffer
	 * 
	 * @param pSamples Pointer to buffer to copy samples to. It must be at least getNumberOfSamples() samples in length.
	 * 
	 * @return true 
	 * @return false 
	 * 
	 * You can skip calling samplesAvailable() and just call copySamples which will return false in the same cases
	 * where samplesAvailable() would have returned false.
	 */
	bool copySamples(void*pSamples) const {
		return Microphone_PDM_MCU::copySamples(pSamples);
	}

	/**
	 * @brief Overrides the default clock (CLK) pin (A0). Not supported on RTL872x (P2, Photon 2)!
	 * 
	 * @param clkPin Pin (such as A2, D3, etc.)
	 * @return Microphone_PDM& 
	 * 
	 * This setting is only used on nRF52 and must be set before init().
	 * 
	 * On the RTL872x (P2, Photon 2), the setting is ignored as you cannot override the pins used for PDM.
	 */
	Microphone_PDM &withPinCLK(pin_t clkPin) { this->clkPin = clkPin; return *this; };

	/**
	 * @brief Overrides the default data (DAT) pin (A1). Not supported on RTL872x (P2, Photon 2)!
	 * 
	 * @param clkPin Pin (such as A2, D3, etc.)
	 * @return Microphone_PDM& 
	 * 
	 * This setting is only used on nRF52 and must be set before init().
	 * 
	 * On the RTL872x (P2, Photon 2), the setting is ignored as you cannot override the pins used for PDM.
	 */
	Microphone_PDM &withPinDAT(pin_t datPin) { this->datPin = datPin; return *this; };
	
#if 0
	/**
	 * @brief Sets the gain in dB
	 *
	 * @param gainDb Gain in dB, from -20 (minimum) to +20 (maximum) in 0.5 dB steps
	 */
	Microphone_PDM &withGainDb(float gainDb);

	/**
	 * @brief Sets the PDM gain using an rRF52 configuration value
	 *
	 * @param gainL The nRF52 gain value for the left or mono channel

	 * @param gainR The nRF52 gain value for the right channel
	 *
	 * - NRF_PDM_GAIN_MINIMUM (0x00)
	 * - NRF_PDM_GAIN_DEFAULT (0x28) 0 dB gain, default value
	 * - NRF_PDM_GAIN_MAXIMUM (0x50)
	 */
	Microphone_PDM &withGain(nrf_pdm_gain_t gainL, nrf_pdm_gain_t gainR) { this->gainL = gainL; this->gainR = gainR; return *this; };


	/**
	 * @brief Set stereo mode/mono mode. Default is mono.
	 * 
	 * @param useStereo Use stereo mode if true. Use mono mode if false.
	 * 
	 * In stereo mode, each buffer will interleave L and R samples. Default is mono mode, and every sample is for the single channel.
	 */
	Microphone_PDM &withStereoMode(bool useStereo = true) { this->stereoMode = useStereo; return *this; };

	/**
	 * @brief Sets the sampling frequency.  Default is 1.032 MHz, which works out to be a PCM sample rate of 16125 samples/sec.
	 *
	 * @param freq Sampling frequency: NRF_PDM_FREQ_1000K, NRF_PDM_FREQ_1032K (default), or NRF_PDM_FREQ_1067K
	 */
	Microphone_PDM &withFreq(nrf_pdm_freq_t freq) { this->freq = freq; return *this; };

	/**
	 * @brief Sets the edge mode
	 *
	 * @param edge Edge mode. Whether left or mono channel is sample on falling CLK (default) or rising CLK
	 *
	 * - NRF_PDM_EDGE_LEFTFALLING Left (or mono) is sampled on falling edge of PDM_CLK (default)
     * - NRF_PDM_EDGE_LEFTRISING Left (or mono) is sampled on rising edge of PDM_CLK.
	 *
	 */
	Microphone_PDM &withEdge(nrf_pdm_edge_t edge) { this->edge = edge; return *this; };
#endif
	/**
	 * @brief Sets the size of the output samples
	 *
	 * @param outputSize The output size enumeration
	 *
	 * - UNSIGNED_8     Output unsigned 8-bit values (adjusted by PDMRange)
	 * - SIGNED_16,	    Output signed 16-bit values (adjusted by PDMRange) (default)
	 * - RAW_SIGNED_16  Output values as signed 16-bit values as returned by nRF52 (unadjusted)
	 *
	 */
	Microphone_PDM &withOutputSize(OutputSize outputSize) { this->outputSize = outputSize; return *this; };

	/**
	 * @brief Sets the range of the output samples
	 *
	 * @param range The range enumeration
	 *
	 * RANGE_128   From -128 to 127 (8 bits)
	 * RANGE_256   From -256 to 255 (9 bits)
	 * RANGE_512   From -512 to 511 (10 bits)
	 * RANGE_1024  From -1024 to 1023 (11 bits)
	 * RANGE_2048  From -2048 to 2047 (12 bits) (default)
	 * RANGE_4096  From -4096 to 4095 (13 bits)
	 * RANGE_8192  From -8192 to 8191 (14 bits)
	 * RANGE_16384 From -16384 to 16383 (15 bits)
	 * RANGE_32768 From -32768 to 32767 (16 bits) (same as raw mode)
	 *
	 */
	Microphone_PDM &withRange(Range range) { this->range = range; return *this; };

	/**
	 * @brief Sets a callback function to be called at interrupt time
	 */
	Microphone_PDM &withInterruptCallback(std::function<void(void *sampleBuf, size_t numSamples)> interruptCallback) { this->interruptCallback = interruptCallback; return *this; };

	/**
	 * @brief Call this to poll for available samples from loop()
	 *
	 * @return A point to a sample buffer or NULL if no samples are available
	 *
	 * Be sure to call doneWithSamples() if a non-NULL value is returned!
	 */
	void *getAvailableSamples() const { return (void *)availableSamples; };

	/**
	 * @brief Call when you are done using the samples returns from getAvailableSamples(). Required!
	 *
	 * This allows the buffer to be reused. If you don't call this, data will be corrupted.
	 */
	void doneWithSamples() { availableSamples = NULL; };

	/**
	 * @brief Returns the number of samples (not bytes) that was returned from getAvailableSamples()
	 */
	size_t getSamplesPerBuf() const { return BUFFER_SIZE_SAMPLES; };


protected:
	/**
	 * @brief Allocate a Microphone_PDM object for using the hardware PDM decoder
	 *
	 */
	Microphone_PDM();

	/**
	 * @brief Destructor. You typically allocate this as a global variable so it won't be deleted.
	 */
	virtual ~Microphone_PDM();

    /**
     * This class is a singleton and cannot be copied
     */
    Microphone_PDM(const Microphone_PDM&) = delete;

    /**
     * This class is a singleton and cannot be copied
     */
    Microphone_PDM& operator=(const Microphone_PDM&) = delete;

	/**
	 * @brief Used internally. Use copySamples() externally
	 * 
	 * @param src 
	 * @param dst 
	 */
	void copySamplesInternal(const int16_t *src, void *dst);


	pin_t clkPin = A0;		//!< The pin used for the PDM clock (output)
	pin_t datPin = A1;		//!< The pin used for the PDM data (input)
	bool stereoMode = false;	//!< Use stereo mode (default: false, mono mode)
#if 0
	nrf_pdm_gain_t gainL = NRF_PDM_GAIN_DEFAULT; 	//!< 0x28 = 0dB gain
	nrf_pdm_gain_t gainR = NRF_PDM_GAIN_DEFAULT; 	//!< 0x28 = 0dB gain
	nrf_pdm_freq_t freq = NRF_PDM_FREQ_1032K;		//!< clock frequency
	nrf_pdm_edge_t edge = NRF_PDM_EDGE_LEFTFALLING; //!< clock edge configuration
#endif
	OutputSize outputSize = OutputSize::SIGNED_16;	//!< Output size (8 or 16 bits)
	Range range = Range::RANGE_2048;				//!< Range adjustment factor
	std::function<void(void *sampleBuf, size_t numSamples)> interruptCallback = NULL;	//!< buffer completion callback
	bool useBufferA = true;							//!< Which buffer we're reading from of the double buffers
	int16_t *availableSamples = NULL;				//!< Used to pass data to loop() for processing out of interrupt context

	/**
	 * @brief Singleton instance of this class
	 *
	 * Since there is only one PDM decoder on the nRF52 you must create only one instance of this class
	 * (typically as a global variable).
	 */
	static Microphone_PDM *_instance;
};



#endif /* __Microphone_PDM_H */
