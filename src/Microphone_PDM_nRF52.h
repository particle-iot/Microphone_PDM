#pragma once

#include "nrfx_pdm.h"


/**
 * @brief MCU-specific implementation of PDM for the nRF52
 * 
 * This class is used on the Boron, B Series SoM, Tracker, and Argon.
 * 
 * You do not instantiate this class directly; it's automatically created when you use the 
 * Microphone_PDM singleton.
 */
class Microphone_PDM_nRF52  : public Microphone_PDM_Base
{
public:
    static const size_t BUFFER_SIZE_SAMPLES = 512; //!< 1024 bytes per buffer
    static const size_t NUM_BUFFERS = 2;  //!< 2 buffers, so 2048 bytes total


protected:
	/**
	 * @brief This object is constructed when the Microphone_PDM class singleton is instantiated
	 */
    Microphone_PDM_nRF52();

	/**
	 * @brief This class is never deleted
	 */
    virtual ~Microphone_PDM_nRF52();

	/**
	 * @brief Initialize the PDM module.
	 *
	 * This is often done from setup(). You can defer it until you're ready to sample if desired,
	 * calling right before start().
	 */
	virtual int init();

	/**
	 * @brief Uninitialize the PDM module.
	 *
	 * You normally will just initialize it once and only start and stop it as necessary, however
	 * you can completely uninitialize it if desired. The clkPin will be reset to INPUT mode.
	 */
	virtual int uninit();

	/**
	 * @brief Start sampling
	 */
	virtual int start();

	/**
	 * @brief Stop sampling
	 */
	virtual int stop();
    

	/**
	 * @brief Return true if there is data available to be copied using copySamples
	 * 
	 * @return true 
	 * @return false 
	 */
	virtual bool samplesAvailable() const;

	/**
	 * @brief Copy samples from the DMA buffer to your buffer
	 * 
	 * @param pSamples Pointer to buffer to copy samples to. It must be at least getNumberOfSamples() samples in length.
	 * 
	 * @return true There were samples available and they were copied.
	 * @return false There were no samples available. Your buffer is unmodified.
	 * 
     * The size of the buffer in bytes will depend on the outputSize. If UNSIGNED_8, then it's getNumberOfSamples() bytes.
     * If SIGNED_16 or RAW_SIGNED_16, then it's 2 * getNumberOfSamples(). 
     * 
	 * You can skip calling samplesAvailable() and just call copySamples which will return false in the same cases
	 * where samplesAvailable() would have returned false.
	 */
	virtual bool copySamples(void* pSamples);

	
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
	virtual bool noCopySamples(std::function<void(void *pSamples, size_t numSamples)>callback);

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
	 */	
	size_t getNumberOfSamples() const {
		return BUFFER_SIZE_SAMPLES / copySrcIncrement();
	}

protected:
	/**
	 * @brief How much to increment src in copySamplesInternal. Used internally.
	 * 
	 * @return size_t number of bytes
	 * 
	 * This is almost always 1. The exception is if you are using 8000 Hz sampling on the nRF52. In this case,
	 * every other sample is skipped and the subclass overrides this to return 2.
	 * 
	 * Override of virtual method in base class Microphone_PDM_Base
	 */
	size_t copySrcIncrement() const;

private:
	/**
	 * @brief Used internally to handle notifications from the PDM peripheral
	 */
	void dataHandler(nrfx_pdm_evt_t const * const pEvent);

	/**
	 * @brief Used internally to handle notifications from the PDM peripheral (static function)
	 *
	 * As this relies on the singleton (instance), there can only be one instance of this class,
	 * however that's also the case because there is only one PDM peripheral on the nRF52.
	 */
	static void dataHandlerStatic(nrfx_pdm_evt_t const * const pEvent);

	nrf_pdm_gain_t gainL = NRF_PDM_GAIN_DEFAULT; 	//!< 0x28 = 0dB gain
	nrf_pdm_gain_t gainR = NRF_PDM_GAIN_DEFAULT; 	//!< 0x28 = 0dB gain
	nrf_pdm_freq_t freq = NRF_PDM_FREQ_1032K;		//!< clock frequency
	nrf_pdm_edge_t edge = NRF_PDM_EDGE_LEFTFALLING; //!< clock edge configuration

	int16_t *currentSampleAvailable = NULL;
	bool useBufferA = true;							//!< Which buffer we're reading from of the double buffers

	int16_t samples[BUFFER_SIZE_SAMPLES * NUM_BUFFERS];
};

/**
 * @brief Microphone_PDM_MCU is an alias for the MCU-specific class like Microphone_PDM_nRF52
 * 
 * This class exists so the subclass Microphone_PDM can just reference Microphone_PDM_MCU
 * as its superclass regardless of which class is actually used.
 */
class Microphone_PDM_MCU : public Microphone_PDM_nRF52 {
};
