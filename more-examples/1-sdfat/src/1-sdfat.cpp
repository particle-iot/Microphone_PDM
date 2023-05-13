#include "Microphone_PDM.h"

#include "SdFatSequentialFileRK.h"
#include "SdFatWavRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;


const int SD_CHIP_SELECT = A5;
SdFat sd;
PrintFile curFile;

// This creates unique, sequentially numbered files on the SD card.
SdFatSequentialFile sequentialFile(sd, SD_CHIP_SELECT, SPI_FULL_SPEED);

// This writes wav files to SD card
// It's actually 16025. You could set that here if instead.
SdFatWavWriter wavWriter(1, 16000, 16);

// CLK = A2, DATA = A3 here, but you can use any available GPIO
// Allocate a PDM decoder. You can only have one per device. In this case, it has 512 int16_t samples
// for each of the double buffers, so this will use 2048 bytes of RAM.
Microphone_PDMStatic<1024> pdmDecoder(A2, A3);

// If you don't hit the setup button to stop recording, this is how long to go before turning it
// off automatically. The limit really is only the disk space available to receive the file.
const unsigned long MAX_RECORDING_LENGTH_MS = 5 * 60 * 1000;

unsigned long recordingStart;

enum State { STATE_WAITING, STATE_OPEN_FILE, STATE_RUNNING, STATE_FINISH };
State state = STATE_WAITING;

// Forward declarations
void buttonHandler(system_event_t event, int data);
bool openNewFile();


void setup() {
	// Register handler to handle clicking on the SETUP button
	System.on(button_click, buttonHandler);

	// Blue D7 LED indicates recording is on
	pinMode(D7, OUTPUT);

	// Optional, just for testing so I can see the logs below
	waitFor(Serial.isConnected, 10000);

	// Save files in the top-level audio directory (created if needed)
	// Files are 000000.wav, 000001.wav, ...
	sequentialFile
		.withDirName("audio")
		.withNamePattern("%06d.wav");

	pdmDecoder.withOutputSize(Microphone_PDM::OutputSize::SIGNED_16);

	// My microphone makes samples from around -2048 to 2047, adjust that so it
	// uses more of the 16-bit range
	pdmDecoder.withRange(Microphone_PDM::Range::RANGE_2048);


	nrfx_err_t err = pdmDecoder.init();
	if (err) {
		Log.error("pdmDecoder.init err=%lu", err);
	}

	err = pdmDecoder.start();
	if (err) {
		Log.error("pdmDecoder.start err=%lu", err);
	}

}

void loop() {
	int16_t *samples;

	switch(state) {
	case STATE_WAITING:
		// Waiting for the user to press the SETUP button. The setup button handler
		// will bump the state into STATE_CONNECT
		break;

	case STATE_OPEN_FILE:
		// Ready to connect to the server via TCP
		if (openNewFile()) {
			// Connected
			Log.info("starting");

			recordingStart = millis();
			digitalWrite(D7, HIGH);

			state = STATE_RUNNING;
		}
		else {
			Log.info("failed to write to SD card");
			state = STATE_WAITING;
		}
		break;

	case STATE_RUNNING:
		samples = (int16_t *)pdmDecoder.getAvailableSamples();

		if (samples) {
			size_t numSamples = pdmDecoder.getSamplesPerBuf();

			// The wav file is configured for signed 16-bit little endian samples already, so
			// there no data translation required
			curFile.write((uint8_t *)samples, 2 * numSamples);

			pdmDecoder.doneWithSamples();
		}


		if (millis() - recordingStart >= MAX_RECORDING_LENGTH_MS) {
			state = STATE_FINISH;
		}
		break;

	case STATE_FINISH:
		Log.info("stopping");
		digitalWrite(D7, LOW);

		// Write the actual length to the wave file header
		wavWriter.updateHeaderFromLength(&curFile);

		// Close file
		curFile.close();

		state = STATE_WAITING;
		break;
	}
}


// button handler for the SETUP button, used to toggle recording on and off
void buttonHandler(system_event_t event, int data) {
	switch(state) {
	case STATE_WAITING:
		state = STATE_OPEN_FILE;
		break;

	case STATE_RUNNING:
		state = STATE_FINISH;
		break;
	}
}

bool openNewFile() {
	bool success = false;

	Log.info("generating sequential file!");

	// Open the next sequential file
	if (sequentialFile.openFile(&curFile, true)) {
		char name[14];
		curFile.getName(name, sizeof(name));

		if (wavWriter.startFile(&curFile)) {
			Log.info("file opened successfully %s", name);

			success = true;
		}
		else {
			Log.info("could not write header");
		}
	}
	else {
		Log.info("file open failed");
	}
	return success;
}






