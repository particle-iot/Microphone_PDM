
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

size_t Microphone_PDM_Base::getSampleSizeInBytes() const {
	switch(outputSize) {
		case OutputSize::UNSIGNED_8:
			return 1;

		default:
			return 2;
	}
}


void Microphone_PDM_Base::copySamplesInternal(const int16_t *src, void *dst) const {
	if (outputSize == OutputSize::UNSIGNED_8) {

		// Scale the 16-bit signed values to an appropriate range for unsigned 8-bit values
		int16_t div = (int16_t)(1 << (size_t) range);

		for(size_t ii = 0; ii < numSamples; ii++) {
			int16_t val = src[ii] / div;

			// Clip to signed 8-bit
			if (val < -128) {
				val = -128;
			}
			if (val > 127) {
				val = 127;
			}

			// Add 128 to make unsigned 8-bit (offset)
			((uint8_t *)dst)[ii] = (uint8_t) (val + 128);
		}

	}
	else if (outputSize == OutputSize::SIGNED_16) {		
		int32_t mult = (int32_t)(1 << (8 - (size_t) range));

		for(size_t ii = 0; ii < numSamples; ii++) {
			// Scale to signed 16 bit range
			int32_t val = (int32_t)src[ii] * mult;

			// Clip to signed 16-bit
			if (val < -32767) {
				val = -32767;
			}
			if (val > 32768) {
				val = 32868;
			}

			((int16_t *)dst)[ii] = (int16_t) val;
		}
	}
	else {
		// OutputSize::RAW_SIGNED_16
		if (src != dst) {
			for(size_t ii = 0; ii < numSamples; ii++) {
				((int16_t *)dst)[ii] = src[ii];
			}
		}
	}
}


