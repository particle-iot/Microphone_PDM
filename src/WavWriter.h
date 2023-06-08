#ifndef _WAVWRITER_H
#define _WAVWRITER_H

#include "Particle.h"
#include "Microphone_PDM.h"

/**
 * @brief Class for manipulating wav file headers
 */
class WavHeaderBase {
public:
	WavHeaderBase(uint8_t *buffer, size_t bufferSize);
	virtual ~WavHeaderBase();

	/**
	 * @brief Writes the wav file header to the start of buffer and updates bufferOffset
	 *
	 * @param numChannels number of channels, typically 1 or 2
	 *
	 * @param sampleRate sampling rate per channel in samples per second. For example: 16000 is 16 kHz sampling rate.
	 *
	 * @param bitsPerSample the number of bits per sample per channel. Typically 8 or 16.
	 *
	 * @param dataSizeInBytes the size of the data (optional). If you don't pass this parameter or pass 0 you
	 * must call setDataSize later, as wav files must include the chunk length and the file length in the header.
	 * There is no facility for files bounded by the length of the file or stream.
	 */
	bool writeHeader(uint8_t numChannels, uint32_t sampleRate, uint8_t bitsPerSample, uint32_t dataSizeInBytes = 0);

	/**
	 * @brief Update the size of the data chunk (in bytes).
	 *
	 * @param dataSizeInBytes This is the size of the data (part of the file after getDataOffset()), which is
	 * typically 44 bytes less than the file size for files we create.
	 *
	 * This modifies the file chunk header and the data subchunk size.
	 */
	void setDataSize(uint32_t dataSizeInBytes);

	/**
	 * @brief Gets the offset of the data chunk
	 *
	 * When we create the file with writeHeader, this will be 44. When we read files, it's typically 44
	 * but could be larger probably.
	 */
	uint32_t getDataOffset() const;

	/**
	 * @brief Find a subchunk within the header
	 *
	 * @param id The subchunk id to find (typically 'fmt ' or 'data')
	 *
	 * @param chunkDataOffset Filled in with the file offset of this subchunk data (not including the header)
	 *
	 * @param chunkDataSize Filled in with the size of this subchunk data (not including the header)
	 *
	 * The entire header must be in buffer. This is usually 44 bytes, but files we didn't write could
	 * be larger with more subchunks.
	 */
	bool findChunk(uint32_t id, size_t &chunkDataOffset, uint32_t &chunkDataSize) const;

	void setUint16LE(size_t offset, uint16_t value);
	uint16_t getUint16LE(size_t offset) const;

	void setUint32LE(size_t offset, uint32_t value);
	uint32_t getUint32LE(size_t offset) const;

	static uint32_t fourCharStringToValue(const char *str);

	void setUint32BE(size_t offset, uint32_t value);
	uint32_t getUint32BE(size_t offset) const;

	size_t getBufferOffset() const { return bufferOffset; };

	uint8_t *getBuffer() { return buffer; };

	const uint8_t *getBuffer() const { return buffer; };

	size_t getBufferSize() const { return bufferSize; };

	/**
	 * @brief This is the size of the header we write using writeHeader.
	 *
	 * The buffer must be at least this large. For reading headers it will also often be 44
	 * bytes but it could be larger. Though some larger files (non-PCM files, for example)
	 * cannot be read. However, it could have extra subchunks, which the library will
	 * safely ignore.
	 */
	static const size_t STANDARD_SIZE = 44;

protected:
	uint8_t *buffer;
	size_t bufferSize;
	size_t bufferOffset = 0;
};

/**
 * @brief Templated class that allows the size of the buffer to be configured
 *
 * For writing wav headers, it must be at least 44 bytes.
 */
template <size_t BUFFER_SIZE>
class WavHeader : public WavHeaderBase {
public:
	explicit WavHeader() : WavHeaderBase(staticBuffer, BUFFER_SIZE) {};

private:
	uint8_t staticBuffer[BUFFER_SIZE]; //!< static buffer to write to
};


class Microphone_PDM_BufferSampling_wav : public Microphone_PDM_BufferSampling {
public:
	Microphone_PDM_BufferSampling_wav();

	virtual void preCompletion();
};


#endif /* _WAVWRITER_H */
