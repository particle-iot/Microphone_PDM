#include "MicWavWriter.h"


// Define the debug logging level here
// 0 = Off
// 1 = Normal
// 2 = High
#define WAV_LOGHANDLER_DEBUG_LEVEL 1

// Don't change these, just change the debugging level above
// Note: must use Serial.printlnf here, not Log.info, as these are called from the log handler itself!
#if WAV_LOGHANDLER_DEBUG_LEVEL >= 1
#define DEBUG_NORMAL(x) Log.info x
#else
#define DEBUG_NORMAL(x)
#endif

#if WAV_LOGHANDLER_DEBUG_LEVEL >= 2
#define DEBUG_HIGH(x) Log.info x
#else
#define DEBUG_HIGH(x)
#endif

MicWavHeaderBase::MicWavHeaderBase(uint8_t *buffer, size_t bufferSize) : buffer(buffer), bufferSize(bufferSize) {

}
MicWavHeaderBase::~MicWavHeaderBase() {

}

bool MicWavHeaderBase::writeHeader(uint8_t numChannels, uint32_t sampleRate, uint8_t bitsPerSample, uint32_t dataSizeInBytes) {
	if (bufferSize < 44) {
		DEBUG_NORMAL(("buffer too small, was %d need 44", bufferSize));
		return false;
	}
	// Chunk ID (4 bytes)
	// Technically using 'riff' would work, but it generates a compiler warning
	setUint32BE(0, fourCharStringToValue("RIFF"));

	// ChunkSize. 36 is the header (44 bytes), except for the first 8 bytes (Chunk ID and Chunk Size) (4 bytes)
	setUint32LE(4, dataSizeInBytes + 36);

	// Format (4 bytes)
	setUint32BE(8, fourCharStringToValue("WAVE"));

	// Subchunk 1 ID (4 bytes)
	setUint32BE(12, fourCharStringToValue("fmt "));

	// Subchunk 1 size (16 bytes for PCM, offsets 20 - 36) (4 bytes)
	setUint32LE(16, 16);

	// Audio format PCM = 1 (2 bytes)
	setUint16LE(20, 1);

	// Num channels (2 bytes)
	setUint16LE(22, numChannels);

	// Sample rate (4 bytes)
	setUint32LE(24, sampleRate);

	// Byte rate (4 bytes)
	setUint32LE(28, sampleRate * numChannels * bitsPerSample / 8);

	// Block align (2 bytes)
	setUint16LE(32, numChannels * bitsPerSample / 8);

	// Bits per sample, per channel (2 bytes)
	setUint16LE(34, bitsPerSample);

	// Subchunk 2 ID (4 bytes)
	setUint32BE(36, fourCharStringToValue("data"));

	// data size (numSamples * numChannels * bitsPerSample / 8) (4 bytes)
	setUint32LE(40, dataSizeInBytes);

	// End of header is offset 44
	bufferOffset = 44;

	return true;
}


void MicWavHeaderBase::setDataSize(uint32_t dataSizeInBytes) {
	// DEBUG_HIGH(("setDataSize %lu", dataSizeInBytes));

	setUint32LE(4, dataSizeInBytes + 36);
	setUint32LE(40, dataSizeInBytes);
}

uint32_t MicWavHeaderBase::getDataOffset() const {
	size_t chunkDataOffset;
	uint32_t chunkDataSize;

	if (findChunk(fourCharStringToValue("data"), chunkDataOffset, chunkDataSize)) {
		return chunkDataOffset;
	}
	else {
		// Not found
		return 0;
	}
}

bool MicWavHeaderBase::findChunk(uint32_t id, size_t &chunkDataOffset, uint32_t &chunkDataSize) const {
	// 12 is start of subchunk 1 for all RIFF WAVE files
	size_t offset = 12;

	while(offset < bufferOffset) {
		uint32_t subChunkSize = getUint32LE(offset + 4);

		uint32_t foundId = getUint32BE(offset);
		if (foundId == id) {
			// Found it! Return the offset of the actual data in the chunk
			chunkDataOffset = offset + 8;
			chunkDataSize = subChunkSize;
			return true;
		}

		offset += subChunkSize + 8;
	}
	return false;
}

// The LE and BE functions assume current Particle ARM Cortex processors that are little
// endian. This could probably be made more portable.

void MicWavHeaderBase::setUint16LE(size_t offset, uint16_t value) {
	// Currently all of the fields in a wav file appear to be aligned, but using
	// memcpy is safer and makes sure we won't get an unaligned exception in these functions
	memcpy(&buffer[offset], &value, 2);
}

uint16_t MicWavHeaderBase::getUint16LE(size_t offset) const {
	uint16_t result;
	memcpy(&result, &buffer[offset], 2);
	return result;
}

void MicWavHeaderBase::setUint32LE(size_t offset, uint32_t value) {
	memcpy(&buffer[offset], &value, 4);
}

uint32_t MicWavHeaderBase::getUint32LE(size_t offset) const {
	uint32_t result;
	memcpy(&result, &buffer[offset], 4);
	return result;
}

// [static]
uint32_t MicWavHeaderBase::fourCharStringToValue(const char *str)  {
	uint32_t value = 0;

	value |= ((uint32_t)str[0]) << 24;
	value |= ((uint32_t)str[1]) << 16;
	value |= ((uint32_t)str[2]) << 8;
	value |= ((uint32_t)str[3]);

	return value;
}

void MicWavHeaderBase::setUint32BE(size_t offset, uint32_t value) {
	buffer[offset]     = (uint8_t) (value >> 24);
	buffer[offset + 1] = (uint8_t) (value >> 16);
	buffer[offset + 2] = (uint8_t) (value >> 8);
	buffer[offset + 3] = (uint8_t) value;
}

uint32_t MicWavHeaderBase::getUint32BE(size_t offset) const {
	uint32_t value = 0;

	value |= ((uint32_t)buffer[offset]) << 24;
	value |= ((uint32_t)buffer[offset + 1]) << 16;
	value |= ((uint32_t)buffer[offset + 2]) << 8;
	value |= ((uint32_t)buffer[offset + 3]);

	return value;
}

/*
From: http://soundfile.sapp.org/doc/WaveFormat/

0         4   ChunkID          Contains the letters "RIFF" in ASCII form
                               (0x52494646 big-endian form).
4         4   ChunkSize        36 + SubChunk2Size, or more precisely:
                               4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
                               This is the size of the rest of the chunk
                               following this number.  This is the size of the
                               entire file in bytes minus 8 bytes for the
                               two fields not included in this count:
                               ChunkID and ChunkSize.
8         4   Format           Contains the letters "WAVE"
                               (0x57415645 big-endian form).

                               The "WAVE" format consists of two subchunks: "fmt " and "data":
The "fmt " subchunk describes the sound data's format:

12        4   Subchunk1ID      Contains the letters "fmt "
                               (0x666d7420 big-endian form).
16        4   Subchunk1Size    16 for PCM.  This is the size of the
                               rest of the Subchunk which follows this number.
20        2   AudioFormat      PCM = 1 (i.e. Linear quantization)
                               Values other than 1 indicate some
                               form of compression.
22        2   NumChannels      Mono = 1, Stereo = 2, etc.
24        4   SampleRate       8000, 44100, etc.
28        4   ByteRate         == SampleRate * NumChannels * BitsPerSample/8
32        2   BlockAlign       == NumChannels * BitsPerSample/8
                               The number of bytes for one sample including
                               all channels. I wonder what happens when
                               this number isn't an integer?
34        2   BitsPerSample    8 bits = 8, 16 bits = 16, etc.
          2   ExtraParamSize   if PCM, then doesn't exist
          X   ExtraParams      space for extra parameters

The "data" subchunk contains the size of the data and the actual sound:

36        4   Subchunk2ID      Contains the letters "data"
                               (0x64617461 big-endian form).
40        4   Subchunk2Size    == NumSamples * NumChannels * BitsPerSample/8
                               This is the number of bytes in the data.
                               You can also think of this as the size
                               of the read of the subchunk following this
                               number.
44        *   Data             The actual sound data.
*/



Microphone_PDM_BufferSampling_wav::Microphone_PDM_BufferSampling_wav() {
	reserveHeaderSize = MicWavHeaderBase::STANDARD_SIZE;
}

void Microphone_PDM_BufferSampling_wav::preCompletion() {
	MicWavHeaderBase wav(buffer, bufferSize);

	wav.writeHeader(
		Microphone_PDM::instance().getNumChannels(),
		(uint32_t) Microphone_PDM::instance().getSampleRate(),
		Microphone_PDM::instance().getBitsPerSample(),
		bufferSize - reserveHeaderSize);
}
