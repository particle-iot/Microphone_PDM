#pragma once


class Microphone_PDM_RTL872x 
{
public:
    static const size_t BUFFER_SIZE_SAMPLES = 256; //!< 512 bytes per buffer
    static const size_t NUM_BUFFERS = 4;  //!< 4 buffers, so 2048 bytes total

protected:
    Microphone_PDM_RTL872x();
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
	virtual int stop() {
        return SYSTEM_ERROR_NOT_SUPPORTED;
    }


	/**
	 * @brief Return true if there is data available to be copied using copySamples
	 * 
	 * @return true 
	 * @return false 
	 */
	bool samplesAvailable() const {
        return false;
	}

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
	bool copySamples(void* pSamples) const {
        return false;
	}
    
};

/**
 * @brief Microphone_PDM_MCU is an alias for the MCU-specific class like Microphone_PDM_RTL872x
 * 
 * This class exists so the subclass Microphone_PDM can just reference Microphone_PDM_MCU
 * as its superclass regardless of which class is actually used.
 */
class Microphone_PDM_MCU : public Microphone_PDM_RTL872x {
};


