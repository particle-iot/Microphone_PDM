#pragma once

#include "rtl_dmic_api.h"

/**
 * @brief MCU-specific implementation of PDM for the RTL872x
 * 
 * This class is used on the P2 and Photon 2.
 * 
 * You do not instantiate this class directly; it's automatically created when you use the 
 * Microphone_PDM singleton.
 */
class Microphone_PDM_RTL872x : public Microphone_PDM_Base
{
public:
    static const size_t BUFFER_SIZE_SAMPLES = SP_DMA_PAGE_SIZE / 2; //!< 512 bytes per buffer
    static const size_t NUM_BUFFERS = SP_DMA_PAGE_NUM;  //!< 4 buffers, so 2048 bytes total

protected:
	/**
	 * @brief This object is constructed when the Microphone_PDM class singleton is instantiated
	 */
    Microphone_PDM_RTL872x();

	/**
	 * @brief This class is never deleted
	 */
    virtual ~Microphone_PDM_RTL872x();

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
	virtual int uninit() {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }

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
	 * On the RTL872x, it's 256 samples (512 bytes). It's smaller because the are 4 buffers instead of the
	 * 2 buffers used on the nRF52, and the optimal DMA size on the RTL872x is 512 bytes.
	 */
	size_t getNumberOfSamples() const {
		return BUFFER_SIZE_SAMPLES;
	}


protected:
	/**
	 * @brief Since the RTL872x cannot stop DMA, this is used to handle start() and stop().
	 */
	bool running = false;

};

/**
 * @brief Microphone_PDM_MCU is an alias for the MCU-specific class like Microphone_PDM_RTL872x
 * 
 * This class exists so the subclass Microphone_PDM can just reference Microphone_PDM_MCU
 * as its superclass regardless of which class is actually used.
 */
class Microphone_PDM_MCU : public Microphone_PDM_RTL872x {
};


