#include "Microphone_PDM.h"


SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;


// If you don't hit the setup button to stop recording, this is how long to go before turning it
// off automatically. The limit really is only the disk space available to receive the file.
const unsigned long MAX_RECORDING_LENGTH_MS = 30000;

// This is the IP Address and port that the server.js node server is running on.
IPAddress serverAddr = IPAddress(192,168,2,4); // **UPDATE THIS**
int serverPort = 7123;

TCPClient client;
unsigned long recordingStart;

enum State { STATE_WAITING, STATE_CONNECT, STATE_RUNNING, STATE_FINISH };
State state = STATE_WAITING;

// Forward declarations
void buttonHandler(system_event_t event, int data);


void setup() {
	// Register handler to handle clicking on the SETUP button
	System.on(button_click, buttonHandler);

	// Blue D7 LED indicates recording is on
	pinMode(D7, OUTPUT);

	// Optional, just for testing so I can see the logs below
	waitFor(Serial.isConnected, 10000);

	// We want the samples converted to unsigned 8-bit, which is what we send over the wire.
	// It's also the standard format for 8-bit wav files.
	// The other common value is Microphone_PDM::OutputSize::SIGNED_16.
	Microphone_PDM::instance().withOutputSize(Microphone_PDM::OutputSize::UNSIGNED_8);

	// My microphone makes samples from around -2048 to 2047, adjust that so it fits in
	// unsigned 8 bit.
	Microphone_PDM::instance().withRange(Microphone_PDM::Range::RANGE_2048);


	int err = Microphone_PDM::instance().init();
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

	case STATE_CONNECT:
		// Ready to connect to the server via TCP
		if (client.connect(serverAddr, serverPort)) {
			// Connected
			Log.info("starting");

			recordingStart = millis();
			digitalWrite(D7, HIGH);

			state = STATE_RUNNING;
		}
		else {
			Log.info("failed to connect to server");
			state = STATE_WAITING;
		}
		break;

	case STATE_RUNNING:
		Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples) {
			client.write((const uint8_t *)pSamples, numSamples);
		});

		if (millis() - recordingStart >= MAX_RECORDING_LENGTH_MS) {
			state = STATE_FINISH;
		}
		break;

	case STATE_FINISH:
		digitalWrite(D7, LOW);
		client.stop();
		Log.info("stopping");
		state = STATE_WAITING;
		break;
	}
}


// button handler for the SETUP button, used to toggle recording on and off
void buttonHandler(system_event_t event, int data) {
	switch(state) {
	case STATE_WAITING:
		state = STATE_CONNECT;
		break;

	case STATE_RUNNING:
		state = STATE_FINISH;
		break;
	}
}






