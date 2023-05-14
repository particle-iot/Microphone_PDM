#include "Particle.h"

#if defined(HAL_PLATFORM_RTL872X) && HAL_PLATFORM_RTL872X

#include "Microphone_PDM.h"

Microphone_PDM_RTL872x::Microphone_PDM_RTL872x() : Microphone_PDM_Base(BUFFER_SIZE_SAMPLES) {

}

Microphone_PDM_RTL872x::~Microphone_PDM_RTL872x() {

}


int Microphone_PDM_RTL872x::init() {
    switch(sampleRate) {
        case 8000:
        case 16000:
        case 32000:
            break;

        default:
            // Invalid value
            sampleRate = 16000;
            break;
    }

    dmic_setup(sampleRate, stereoMode);
    return 0;
}


int Microphone_PDM_RTL872x::start() {
    dmic_flush();

    running = true;
    return 0;
}

int Microphone_PDM_RTL872x::stop() {
    running = false;
    return 0;
}
 


bool Microphone_PDM_RTL872x::samplesAvailable() const {
    if (!running) {
        return false;
    }

	return (dmic_ready() != NULL);
}

bool Microphone_PDM_RTL872x::copySamples(void*pSamples) {
    if (!running) {
        return false;
    }

    int16_t *src = (int16_t *)dmic_ready();
	if (src) {
		copySamplesInternal(src, (uint8_t *)pSamples);
        dmic_read(NULL, 0);
		return true;
	}
	else {
		return false;
	}
}

bool Microphone_PDM_RTL872x::noCopySamples(std::function<void(void *pSamples, size_t numSamples)>callback) {
    if (!running) {
        return false;
    }

    int16_t *src = (int16_t *)dmic_ready();
	if (src) {
		copySamplesInternal(src, (uint8_t *)src);
		callback(src, BUFFER_SIZE_SAMPLES);
        dmic_read(NULL, 0);
		return true;
	}
	else {
		return false;
	}
}


#endif // HAL_PLATFORM_RTL872X
