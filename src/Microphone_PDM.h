#ifndef __Microphone_PDM_H
#define __Microphone_PDM_H

#include "Particle.h"

/**
 * @brief Class used for settings. You will not instantiate one of these; it's a base class of Microphone_PDM_MCU.
 * 
 * There are accessor methods for setting these settings in the Microphone_PDM class.
 */
class Microphone_PDM_Base {
public:
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



protected:
	/**
	 * @brief You cannot instantiate one of these, it's only done by the subclass, which is a Microphone_PDM_* MCU-specific class
	 * 
	 * @param numSamples The value of BUFFER_SIZE_SAMPLES defined in the MCU-specific subclass
	 */
	Microphone_PDM_Base(size_t numSamples) : numSamples(numSamples) {};

	/**
	 * @brief This class is never deleted
	 */
	virtual ~Microphone_PDM_Base() {};

	/**
	 * @brief Used internally. Use copySamples() externally.
	 * 
	 * @param src Pointer to the internal DMA buffer, which is always 16-bit samples
	 * @param dst Pointer to the destination buffer, which will be 8 or 16-bit samples, depending on outputSize
	 * 
	 * Also pays attention to range to determine how much to shift the samples, unless the output size
	 * is RAW_SIGNED_16, which does not do any transformation.
	 * 
	 * src and dst can be the same buffer to transform the data range in place.
	 */
	void copySamplesInternal(const int16_t *src, uint8_t *dst) const;

	/**
	 * @brief How much to increment src in copySamplesInternal. Used internally.
	 * 
	 * @return size_t number of bytes
	 * 
	 * This is almost always 1. The exception is if you are using 8000 Hz sampling on the nRF52. In this case,
	 * every other sample is skipped and the subclass overrides this to return 2.
	 */
	virtual size_t copySrcIncrement() const { return 1; };

	pin_t clkPin = A0;		//!< The pin used for the PDM clock (output)
	pin_t datPin = A1;		//!< The pin used for the PDM data (input)
	bool stereoMode = false;	//!< Use stereo mode (default: false, mono mode)
	int sampleRate; //!< Either 8000 or 16000 only. Not supported on nRF52.
	OutputSize outputSize = OutputSize::SIGNED_16;	//!< Output size (8 or 16 bits)
	Range range = Range::RANGE_2048;				//!< Range adjustment factor
	size_t numSamples; //!< Number of samples in the DMA buffer
};

// This is here because the platform-specific classes derive from Microphone_PDM_Base
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
 * It is a singleton class. You cannot construct a variable of this type, or copy this class. Instead, use
 * Microphone_PDM::instance() to get the singleton instance.
 * 
 * You can create an instance at early initialization (STARTUP or global object construction) but you cannot
 * call init() at that time. In almost all cases, you will first use Microphone_PDM::instance() from setup
 * to configure the settings.
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
	 * @param datPin Pin (such as A3, D4, etc.)
	 * @return Microphone_PDM& 
	 * 
	 * This setting is only used on nRF52 and must be set before init().
	 * 
	 * On the RTL872x (P2, Photon 2), the setting is ignored as you cannot override the pins used for PDM.
	 */
	Microphone_PDM &withPinDAT(pin_t datPin) { this->datPin = datPin; return *this; };

	/**
	 * @brief Sets the size of the output samples
	 *
	 * @param outputSize The output size enumeration
	 *
	 * - UNSIGNED_8     Output unsigned 8-bit values (adjusted by PDMRange)
	 * - SIGNED_16,	    Output signed 16-bit values (adjusted by PDMRange) (default)
	 * - RAW_SIGNED_16  Output values as signed 16-bit values as returned by nRF52 (unadjusted)
	 *
	 * The DMA buffer is always 16 bit, and if you use UNSIGNED_8 it just discards the unused bits
	 * when copying the samples using copySamples() or noCopySamples().
	 * 
	 * This is only relevant because you will be called at the rate you'd expect for 16-bit samples
	 * even when using 8-bit output.
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
	 * The range should be set based on the PDM microphone you are using. For the Adafruit microphone,
	 * the default value of RANGE_2048 (12-bit) is correct. 
	 */
	Microphone_PDM &withRange(Range range) { this->range = range; return *this; };

	/**
	 * @brief Sets the sampling rate. Default is 16000. Cannot be changed on nRF52!
	 *
	 * @param sampleRate 8000, 16000, or 32000. The default is 16000.
	 * 
	 * This call can only be used on RTL827x (P2, Photon 2). It is ignored on nRF52.
	 * Setting an invalid value will use 16000.
	 */
	Microphone_PDM &withSampleRate(int sampleRate) { this->sampleRate = sampleRate; return *this; };

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
	 * start it. Calling stop() will discard the data and samplesAvailable(), copySamples(),
	 * etc. will behave as if the DMA had been stopped.
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
	 * On the nRF52, it's 512 samples (1024 bytes), except in one case: If you set a sample rate of
	 * 8000 Hz, it will be 256 samples because the hardware only samples at 16000 Hz but the code
	 * will automatically discard every other sample so there will only be 256 samples.
	 * 
	 * On the RTL872x, it's 256 samples (512 bytes). It's smaller because the are 4 buffers instead of the
	 * 2 buffers used on the nRF52, and the optimal DMA size on the RTL872x is 512 bytes.
	 */
	size_t getNumberOfSamples() const {
		return Microphone_PDM_MCU::getNumberOfSamples();
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
	bool copySamples(void*pSamples) {
		return Microphone_PDM_MCU::copySamples(pSamples);
	}

	/**
	 * @brief Alternative API to get samples
	 *
	 * @param callback Callback function or lambda
	 *  
	 * @return true 
	 * @return false 
	 * 
	 * Alternative API that does not require a buffer to be passed in. You should only use this if you can
	 * consume the buffer immediately without blocking. 
	 * 
	 * The callback function or lamba has this prototype:
	 * 
	 *   void callback(void *pSamples, size_t numSamples)
	 * 
	 * It will be called with a pointer to the samples (in the DMA buffer) and the number of samples (not bytes!) 
	 * of data. The number of bytes will vary depending on the outputSize. 
	 * 
	 * You can skip calling samplesAvailable() and just call noCopySamples which will return false in the same cases
	 * where samplesAvailable() would have returned false.
	 */
	bool noCopySamples(std::function<void(void *pSamples, size_t numSamples)>callback) {
		return Microphone_PDM_MCU::noCopySamples(callback);
	}

	/**
	 * @brief Get the sample size in bytes
	 * 
	 * @return size_t 1 (8-bit samples) or 2 (16-bit samples)
	 */
	size_t getSampleSizeInBytes() const;

	/**
	 * @brief Get the buffer size in bytes
	 * 
	 * @return size_t Size of the DMA buffer in bytes
	 * 
	 * You can use this with copySamples() to know how big of a buffer you need if you are allocating a
	 * buffer in bytes instead of samples.
	 */
	size_t getBufferSizeInBytes() const {
		return getSampleSizeInBytes() * getNumberOfSamples();
	}


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
	 * @brief Singleton instance of this class
	 *
	 * Since there is only one PDM decoder on the nRF52 you must create only one instance of this class
	 * (typically as a global variable).
	 */
	static Microphone_PDM *_instance;
};



#endif /* __Microphone_PDM_H */
