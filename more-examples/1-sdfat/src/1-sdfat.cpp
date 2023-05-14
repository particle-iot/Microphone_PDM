#include "Microphone_PDM.h"

#include "SdFatSequentialFileRK.h"
#include "SdFatWavRK.h"

SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;


const int SD_CHIP_SELECT = A2;
SdFat sd;
PrintFile curFile;

// This creates unique, sequentially numbered files on the SD card.
SPISettings spiSettings(12 * MHZ, MSBFIRST, SPI_MODE0);
SdFatSequentialFile sequentialFile(sd, SD_CHIP_SELECT, spiSettings);

// This writes wav files to SD card
// It's actually 16025. You could set that here if instead.
SdFatWavWriter wavWriter(1, 16000, 16);

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

	int err = Microphone_PDM::instance()
		.withOutputSize(Microphone_PDM::OutputSize::SIGNED_16)
		.withRange(Microphone_PDM::Range::RANGE_2048)
		.withSampleRate(16000)
		.init();

	if (err) {
		Log.error("pdmDecoder.init err=%d", err);
	}

	err = Microphone_PDM::instance().start();
	if (err) {
		Log.error("pdmDecoder.start err=%d", err);
	}

}

void loop() {
	
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
		Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples) {
			curFile.write((uint8_t *)pSamples, Microphone_PDM::instance().getBufferSizeInBytes());
		});


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






