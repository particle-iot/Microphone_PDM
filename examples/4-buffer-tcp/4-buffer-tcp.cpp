#include "Microphone_PDM.h"


SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;


// How long to record in milliseconds
const unsigned long RECORDING_LENGTH_MS = 5000;

// This is the IP Address and port that the server.js node server is running on.
IPAddress serverAddr = IPAddress(192,168,2,6); // **UPDATE THIS**
int serverPort = 7123;

TCPClient client;
unsigned long recordingStart;
Microphone_PDM_BufferSampling *samplingBuffer;

enum State { STATE_WAITING, STATE_START_RECORDING, STATE_RECORDING_WAIT, STATE_CONNECT, STATE_TRANSMIT, STATE_FINISH };
State state = STATE_WAITING;

// Forward declarations
void buttonHandler(system_event_t event, int data);


void setup() {
	Particle.connect();

	// Register handler to handle clicking on the SETUP button
	System.on(button_click, buttonHandler);

	// Blue D7 LED indicates recording is on
	pinMode(D7, OUTPUT);
	digitalWrite(D7, LOW);

	// Optional, just for testing so I can see the logs below
	// waitFor(Serial.isConnected, 10000);

	// output size is typically Microphone_PDM::OutputSize::UNSIGNED_8 or icrophone_PDM::OutputSize::SIGNED_16
	// and must match what the server is expecting (set on the command line)

	// Range must match the microphone. The Adafruit PDM microphone is 12-bit, so Microphone_PDM::Range::RANGE_2048.

	// The sample rate default to 16000. The other valid value is 8000.

	int err = Microphone_PDM::instance()
		.withOutputSize(Microphone_PDM::OutputSize::SIGNED_16)
		.withRange(Microphone_PDM::Range::RANGE_2048)
		.withSampleRate(16000)
		.init();

	if (err) {
		Log.error("PDM decoder init err=%d", err);
	}

	err = Microphone_PDM::instance().start();
	if (err) {
		Log.error("PDM decoder start err=%d", err);
	}

}

void loop() {
    Microphone_PDM::instance().loop();

	switch(state) {
	case STATE_WAITING:
		// Waiting for the user to press the SETUP button. The setup button handler
		// will bump the state into STATE_CONNECT
		break;

	case STATE_START_RECORDING:
		state = STATE_RECORDING_WAIT;
	    digitalWrite(D7, HIGH);

        samplingBuffer = new Microphone_PDM_BufferSampling();

        samplingBuffer->withCompletionCallback([](uint8_t *buf, size_t bufSize) {
            Log.info("done! buf=%x size=%d", (int)buf, (int)bufSize);
    	    digitalWrite(D7, LOW);


			state = STATE_CONNECT;
        });
        samplingBuffer->withDurationMs(RECORDING_LENGTH_MS);

        Microphone_PDM::instance().bufferSamplingStart(samplingBuffer);
		Log.info("sampling started");
		break;

	case STATE_RECORDING_WAIT:
		break;

	case STATE_CONNECT:
		// Ready to connect to the server via TCP
		Log.info("connecting...");
		if (client.connect(serverAddr, serverPort)) {
			// Connected
			Log.info("transmitting");


			state = STATE_TRANSMIT;
		}
		else {
			Log.info("failed to connect to server");
			state = STATE_WAITING;
		}
		break;

	case STATE_TRANSMIT:
		client.write(samplingBuffer->buffer, samplingBuffer->bufferSize);
		Log.info("transmit complete!");
		state = STATE_FINISH;
		break;


	case STATE_FINISH:
		client.stop();
		state = STATE_WAITING;
		break;
	}
}


// button handler for the SETUP button, used to toggle recording on and off
void buttonHandler(system_event_t event, int data) {
	switch(state) {
	case STATE_WAITING:
		if (WiFi.ready()) {
			state = STATE_START_RECORDING;
		}
		break;

	}
}






