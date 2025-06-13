#include "Microphone_PDM.h"
#include "MicWavWriter.h"

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;

// From FftComplex.h
// See 

#include <complex>
#include <vector>


namespace Fft {
	
	/* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This is a wrapper function.
	 */
	void transform(std::vector<std::complex<double> > &vec);
	
	
	/* 
	 * Computes the inverse discrete Fourier transform (IDFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This is a wrapper function. This transform does not perform scaling, so the inverse is not a true inverse.
	 */
	void inverseTransform(std::vector<std::complex<double> > &vec);
	
	
	/* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector's length must be a power of 2. Uses the Cooley-Tukey decimation-in-time radix-2 algorithm.
	 */
	void transformRadix2(std::vector<std::complex<double> > &vec);
	
	
	/* 
	 * Computes the discrete Fourier transform (DFT) of the given complex vector, storing the result back into the vector.
	 * The vector can have any length. This requires the convolution function, which in turn requires the radix-2 FFT function.
	 * Uses Bluestein's chirp z-transform algorithm.
	 */
	void transformBluestein(std::vector<std::complex<double> > &vec);
	
	
	/* 
	 * Computes the circular convolution of the given complex vectors. Each vector's length must be the same.
	 */
	void convolve(
		const std::vector<std::complex<double> > &vecx,
		const std::vector<std::complex<double> > &vecy,
		std::vector<std::complex<double> > &vecout);
	
}

const int dtmfLowFreq[4] = { 697, 770, 852, 941 };
const int dtmfHighFreq[4] = { 1209, 1336, 1477, 1633 };

const size_t SAMPLE_FREQ = 8000;
float *floatSamples;
char lastKeyFromISR = 0;
char lastKey = 0;

// First index: low freq 0 - 3
// Second index: high freq 0 - 3
const char dtmfKey[4][4] = {
		{ '1', '2', '3', 'A' },
		{ '4', '5', '6', 'B' },
		{ '7', '8', '9', 'C' },
		{ '*', '0', '#', 'D' }
};

// Forward declaration
char findPressedKey(float *samples, size_t numSamples);
float goertzelMagSquared(int numSamples, int targetFreq, int sampleFreq, const float *data);


void setup() {
	Particle.connect();

	// Blue D7 LED indicates DTMF detected
	pinMode(D7, OUTPUT);
	digitalWrite(D7, LOW);

	// Optional, just for testing so I can see the logs below
	waitFor(Serial.isConnected, 10000);

	// Range must match the microphone. The Adafruit PDM microphone is 12-bit, so Microphone_PDM::Range::RANGE_2048.
	// The sample rate default to 16000. The other valid value is 8000.

    // On nRF52 devices, you can use other pins for PDM
    //  .withPinCLK(A4)
    //  .withPinDAT(A5)


	int err = Microphone_PDM::instance()
		.withOutputSize(Microphone_PDM::OutputSize::SIGNED_16)
		.withRange(Microphone_PDM::Range::RANGE_2048)
		.withSampleRate(SAMPLE_FREQ)
		.init();

	if (err) {
		Log.error("PDM decoder init err=%d", err);
	}

    floatSamples = new float[Microphone_PDM::instance().getNumberOfSamples()];

	err = Microphone_PDM::instance().start();
	if (err) {
		Log.error("PDM decoder start err=%d", err);
	}

}

void loop() {
    Microphone_PDM::instance().loop();

    Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples) {
        // This lambda is called from an ISR and must not allocate memory or
        // perform other prohibited operations.
        const int16_t *src = (const int16_t *)pSamples;

        for(size_t ii = 0; ii < numSamples; ii++) {
			// We are using 12-bit sampling 

			// Map to values from -1.0 to +1.0 as a float instead
			floatSamples[ii] = (float)src[ii] / 2048.0;
		}

		lastKeyFromISR = findPressedKey(floatSamples, numSamples);
    });


    if (lastKeyFromISR) {
    	digitalWrite(D7, HIGH);
        if (lastKey != lastKeyFromISR) {
            lastKey = lastKeyFromISR;
            Log.info("pressed %c", lastKey);
        }
    }
    else {
	    digitalWrite(D7, LOW);
    }

}



char findPressedKey(float *samples, size_t numSamples) {
	// Run the Goertzel algorithm to detect the DTMF frequencies
	// (see note below about this algorithm)
	int lowFreq = -1;
	for(size_t ii = 0; ii < sizeof(dtmfLowFreq) / sizeof(dtmfLowFreq[0]); ii++) {
		int freq = dtmfLowFreq[ii];
		float magSquared = goertzelMagSquared(numSamples, freq, SAMPLE_FREQ, samples);
		if (magSquared > 100.0) {
			lowFreq = (int)ii;
			break;
		}
	}

	int highFreq = -1;
	for(size_t ii = 0; ii < sizeof(dtmfHighFreq) / sizeof(dtmfHighFreq[0]); ii++) {
		int freq = dtmfHighFreq[ii];
		float magSquared = goertzelMagSquared(numSamples, freq, SAMPLE_FREQ, samples);
		if (magSquared > 100.0) {
			highFreq = (int)ii;
			break;
		}
	}

	if (lowFreq >= 0 && highFreq >= 0) {
		return dtmfKey[lowFreq][highFreq];
	}
	else {
		return 0;
	}
}


// One alternative is to use a FFT which will find the magnitude and phase of all
// frequencies (separated into buckets). The other is to use the Goertzel algorithm
// which finds the magnitude of a specific frequency. This is ideal when doing
// DTMF decoding because we have a maximum of 8 frequencies to check, and is more
// efficient in this case because we don't need the magnitude of the frequencies
// we don't care about, and also don't need the phase.
//
// https://en.wikipedia.org/wiki/Goertzel_algorithm
// Modified version of:
// https://stackoverflow.com/questions/11579367/implementation-of-goertzel-algorithm-in-c
float goertzelMagSquared(int numSamples, int targetFreq, int sampleFreq, const float *data) {
	float   omega,sine,cosine,coeff,q0,q1,q2,magnitudeSquared,real,imag;

	int k = (int) (0.5 + (((float)numSamples * targetFreq) / sampleFreq));
	omega = (2.0 * M_PI * k) / (float)numSamples;
	sine = sin(omega);
	cosine = cos(omega);
	coeff = 2.0 * cosine;
	q0=0;
	q1=0;
	q2=0;

	for(int ii=0; ii < numSamples; ii++) {
		q2 = q1;
		q1 = q0;
		q0 = coeff * q1 - q2 + data[ii];
	}

	real = (q0 - q1 * cosine);
	imag = (-q1 * sine);

	magnitudeSquared = real*real + imag*imag;
	return magnitudeSquared;
}









// From FftComplex.cpp
/* 
 * Free FFT and convolution (C++)
 * 
 * Copyright (c) 2019 Project Nayuki. (MIT License)
 * https://www.nayuki.io/page/free-small-fft-in-multiple-languages
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * - The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 * - The Software is provided "as is", without warranty of any kind, express or
 *   implied, including but not limited to the warranties of merchantability,
 *   fitness for a particular purpose and noninfringement. In no event shall the
 *   authors or copyright holders be liable for any claim, damages or other
 *   liability, whether in an action of contract, tort or otherwise, arising from,
 *   out of or in connection with the Software or the use or other dealings in the
 *   Software.
 */

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
//#include "FftComplex.h"

using std::complex;
using std::size_t;
using std::vector;


// Private function prototypes
static size_t reverseBits(size_t x, int n);


void Fft::transform(vector<complex<double> > &vec) {
	size_t n = vec.size();
	if (n == 0)
		return;
	else if ((n & (n - 1)) == 0)  // Is power of 2
		transformRadix2(vec);
	else  // More complicated algorithm for arbitrary sizes
		transformBluestein(vec);
}


void Fft::inverseTransform(vector<complex<double> > &vec) {
	std::transform(vec.cbegin(), vec.cend(), vec.begin(),
		static_cast<complex<double> (*)(const complex<double> &)>(std::conj));
	transform(vec);
	std::transform(vec.cbegin(), vec.cend(), vec.begin(),
		static_cast<complex<double> (*)(const complex<double> &)>(std::conj));
}


void Fft::transformRadix2(vector<complex<double> > &vec) {
	// Length variables
	size_t n = vec.size();
	int levels = 0;  // Compute levels = floor(log2(n))
	for (size_t temp = n; temp > 1U; temp >>= 1)
		levels++;
	if (static_cast<size_t>(1U) << levels != n) {
		// throw std::domain_error("Length is not a power of 2");
		return;
	}
	
	// Trignometric table
	vector<complex<double> > expTable(n / 2);
	for (size_t i = 0; i < n / 2; i++)
		expTable[i] = std::polar(1.0, -2 * M_PI * i / n);
	
	// Bit-reversed addressing permutation
	for (size_t i = 0; i < n; i++) {
		size_t j = reverseBits(i, levels);
		if (j > i)
			std::swap(vec[i], vec[j]);
	}
	
	// Cooley-Tukey decimation-in-time radix-2 FFT
	for (size_t size = 2; size <= n; size *= 2) {
		size_t halfsize = size / 2;
		size_t tablestep = n / size;
		for (size_t i = 0; i < n; i += size) {
			for (size_t j = i, k = 0; j < i + halfsize; j++, k += tablestep) {
				complex<double> temp = vec[j + halfsize] * expTable[k];
				vec[j + halfsize] = vec[j] - temp;
				vec[j] += temp;
			}
		}
		if (size == n)  // Prevent overflow in 'size *= 2'
			break;
	}
}


void Fft::transformBluestein(vector<complex<double> > &vec) {
	// Find a power-of-2 convolution length m such that m >= n * 2 + 1
	size_t n = vec.size();
	size_t m = 1;
	while (m / 2 <= n) {
		if (m > SIZE_MAX / 2) {
			// throw std::length_error("Vector too large");
			return;
		}
		m *= 2;
	}
	
	// Trignometric table
	vector<complex<double> > expTable(n);
	for (size_t i = 0; i < n; i++) {
		unsigned long long temp = static_cast<unsigned long long>(i) * i;
		temp %= static_cast<unsigned long long>(n) * 2;
		double angle = M_PI * temp / n;
		expTable[i] = std::polar(1.0, -angle);
	}
	
	// Temporary vectors and preprocessing
	vector<complex<double> > av(m);
	for (size_t i = 0; i < n; i++)
		av[i] = vec[i] * expTable[i];
	vector<complex<double> > bv(m);
	bv[0] = expTable[0];
	for (size_t i = 1; i < n; i++)
		bv[i] = bv[m - i] = std::conj(expTable[i]);
	
	// Convolution
	vector<complex<double> > cv(m);
	convolve(av, bv, cv);
	
	// Postprocessing
	for (size_t i = 0; i < n; i++)
		vec[i] = cv[i] * expTable[i];
}


void Fft::convolve(
		const vector<complex<double> > &xvec,
		const vector<complex<double> > &yvec,
		vector<complex<double> > &outvec) {
	
	size_t n = xvec.size();
	if (n != yvec.size() || n != outvec.size()) {
		// throw std::domain_error("Mismatched lengths");
		return;
	}
	vector<complex<double> > xv = xvec;
	vector<complex<double> > yv = yvec;
	transform(xv);
	transform(yv);
	for (size_t i = 0; i < n; i++)
		xv[i] *= yv[i];
	inverseTransform(xv);
	for (size_t i = 0; i < n; i++)  // Scaling (because this FFT implementation omits it)
		outvec[i] = xv[i] / static_cast<double>(n);
}


static size_t reverseBits(size_t x, int n) {
	size_t result = 0;
	for (int i = 0; i < n; i++, x >>= 1)
		result = (result << 1) | (x & 1U);
	return result;
}
