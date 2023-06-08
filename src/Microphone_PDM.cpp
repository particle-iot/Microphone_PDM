
#include "Microphone_PDM.h"

// #include "pinmap_hal.h"

// 

Microphone_PDM *Microphone_PDM::_instance = NULL;

Microphone_PDM::Microphone_PDM() {
}

Microphone_PDM::~Microphone_PDM() {

}

// [static] 
Microphone_PDM &Microphone_PDM::instance() {
	if (!_instance) {
		_instance = new Microphone_PDM();
	}
	return *_instance;
}

#if 0
Microphone_PDM &Microphone_PDM::withGainDb(float gain) {
	if (gain < -20.0) {
		gain = -20.0;
	}
	if (gain > 20.0) {
		gain = 20.0;
	}

	int16_t halfSteps = (int16_t)(gain * 2);

	// nRF52 values are from:
	// -20 dB = 0x00
	// 0 dB   = 0x28 = 40
	// +20 dB = 0x50 = 80
	return withGain(halfSteps + 40, halfSteps + 40);
}
#endif



void Microphone_PDM::loop() {
	if (sampling) {
		sampling->loop();
	}

}


bool Microphone_PDM::bufferSamplingStart(Microphone_PDM_BufferSampling *sampling) {
	releaseBufferSampling();

	this->sampling = sampling;

	return sampling->start();
}

/**
 * @brief Release the buffer used by bufferSamplingStart()
 * 
 */
void Microphone_PDM::releaseBufferSampling() {
	if (sampling) {
		delete sampling;
	}
	sampling = NULL;
}

size_t Microphone_PDM::getSampleSizeInBytes() const {
	switch(outputSize) {
		case OutputSize::UNSIGNED_8:
			return 1;

		default:
			return 2;
	}
}


void Microphone_PDM_Base::copySamplesInternal(const int16_t *src, uint8_t *dst) const {
	const int16_t *srcEnd = &src[numSamples];

	size_t increment = copySrcIncrement();

	if (outputSize == OutputSize::UNSIGNED_8) {

		// Scale the 16-bit signed values to an appropriate range for unsigned 8-bit values
		int16_t div = (int16_t)(1 << (size_t) range);

		while(src < srcEnd) {
			int16_t val = *src / div;
			src += increment;

			// Clip to signed 8-bit
			if (val < -128) {
				val = -128;
			}
			if (val > 127) {
				val = 127;
			}

			// Add 128 to make unsigned 8-bit (offset)
			*((uint8_t *)dst) = (uint8_t) (val + 128);
			dst += sizeof(uint8_t);
		}

	}
	else if (outputSize == OutputSize::SIGNED_16) {		
		int32_t mult = (int32_t)(1 << (8 - (size_t) range));

		while(src < srcEnd) {
			// Scale to signed 16 bit range
			int32_t val = (int32_t)*src * mult;
			src += increment;

			// Clip to signed 16-bit
			if (val < -32767) {
				val = -32767;
			}
			if (val > 32768) {
				val = 32868;
			}

			*((int16_t *)dst) = (int16_t) val;
			dst += sizeof(uint16_t);
		}
	}
	else {
		// OutputSize::RAW_SIGNED_16
		if (src != (int16_t *)dst || increment != 1) {
			while(src < srcEnd) {
				*((int16_t *)dst) = *src;
				dst += sizeof(int16_t);
				src += increment;
			}
		}
	}
}



Microphone_PDM_BufferSampling::Microphone_PDM_BufferSampling() {
}

Microphone_PDM_BufferSampling::~Microphone_PDM_BufferSampling() {
	if (buffer) {
		delete[] buffer;		
	}
}

bool Microphone_PDM_BufferSampling::start() {
	sampleSizeInBytes = Microphone_PDM::instance().getSampleSizeInBytes();

	offset = reserveHeaderSize;
	bufferSize = reserveHeaderSize + (Microphone_PDM::instance().getSampleRate() / 1000 * durationMs) * sampleSizeInBytes;

	buffer = new uint8_t[bufferSize];

	return buffer != NULL;
}

void Microphone_PDM_BufferSampling::loop() {
	if (buffer && offset < bufferSize && Microphone_PDM::instance().samplesAvailable()) {

		Microphone_PDM::instance().noCopySamples([this](void *pSamples, size_t numSamples) {
			size_t bytesToCopy = sampleSizeInBytes * numSamples;

			if ((offset + bytesToCopy) > bufferSize) {
				bytesToCopy = bufferSize - offset;
			}
			memcpy(&buffer[offset], pSamples, bytesToCopy);
			offset += bytesToCopy;

			if (offset >= bufferSize && completionCallback) {
				preCompletion();

				completionCallback(buffer, bufferSize);
			}
		});

	}
}
bool Microphone_PDM_BufferSampling::done() const {
	return buffer && offset >= bufferSize;
}
