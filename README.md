# Microphone_PDM

*Library for PDM microphones for RTL872x and nRF52 (P2, Photon 2, Boron, B Series SoM, Tracker SoM, Argon*

Digital MEMS microphones are commonly available in I2S and PDM interface. This library only supports the 
PDM ([pulse-density modulation](https://en.wikipedia.org/wiki/Pulse-density_modulation)) interface. 

PDM MEMS microphones are generally designed for voice use - not for high fidelity audio. 

Adafruit has two breakout boards:

- [For use in a breadboard](https://www.adafruit.com/product/3492) 
- [With JST-SH connector](https://www.adafruit.com/product/4346)

Both are US$4.95 and there is [more information available at Adafruit](https://learn.adafruit.com/adafruit-pdm-microphone-breakout/). The boards include a [MP34DT01-M](https://cdn-learn.adafruit.com/assets/assets/000/049/977/original/MP34DT01-M.pdf) (PDF datasheet).

The pins needed for Adafruit PDM microphones are:

| Breakout Pin | Particle Pin | Description | Color |
| --- | --- | --- | --- |
| 3V | 3V3 | Power 3.3V | Red |
| GND | GND | Ground | Black |
| SEL | | Left/Right Select | No Connection |
| CLK | A0 | PDM Clock | Blue | 
| DAT | A1 | PDM Data | Green | 

![Photon 2](images/argon.jpg)

![Argon](images/argon.jpg)

- SEL - Left/Right select. If this pin is high, the output is on the falling edge of CLK considered the 'Right' channel. If this pin is low, the output is on the rising edge, a.k.a 'Left' channel. Has a pull-down to default to left so you can leave it unconnected.
- CLK - PDM clock into breakout board, 1 - 3 MHz square wave required.
- DAT - PDM data out of breakout board

On the nRF52 (Boron, etc.) the PDM CLK and DAT lines are configurable to other pins, but on RTL872x (P2 and Photon 2), only A0 (CLK) and D1 (DAT) can be used.

On both MCUs, the built-in hardware PDM decoder is used, along with DMA to write to the buffer, so the operation is 
very efficient and does not block the execution of your code while sampling audio.

On the RTL827x, once you start PDM sampling it cannot be stopped without resetting the MCU! You can, however, simply ignore the sampled data, which will be discarded and the sampling only generates a minimal number of interrupts and uses minimal CPU.

- Full [browsable API documentation](https://particle-iot.github.io/Microphone_PDM/)
- Repository: https://github.com/particle-iot/Microphone_PDM
- License: Apache 2 (Free for use. Can be used in closed-source commercial applications. No attribution required.)

## Examples

### Audio over TCP (8-bit)

This example sends the data as 16000 Hz, mono, 8-bit data over TCP to a node.js server. The intention is to use this with an Argon, P2, or Photon 2 over a local Wi-Fi network to a computer on the same network.

- examples/1-audio-over-tcp

When you press the MODE button the transmission will begin and the blue D7 LED will turn on. The recording will go for 30 seconds, or until you press the MODE button again. 

You could easily modify the code to record longer, even indefinitely. It likely will not work  with a Boron over cellular as there is no compression of the data and the data rate is too high. It won't work with a Xenon over mesh because TCP is not supported on a mesh-only node, and also the data rate is too high.

To run the server:

```
cd more-examples/tcp-audio-server
npm install
node app.js --rate 16000 --bits 8
```

When a client connects it creates a new file in the **out** directory. They're sequentially numbered and in wav format.

Make sure you update the device firmware to specify the IP address of your node.js server! It will be printed out when you start the server.


### SdFat Wav 

When you press the MODE button this example records audio to a SD card. Pressing MODE stops recording. Each press creates a new, sequentially numbered file in the **audio** directory on the SD card.

- more-examples/1-sdfat

This example requires a lot of parts:

- A compatible Particle devices
- SD card reader (connected by SPI)
- PDM microphone

## Version History

#### 0.0.1 (2023-05-12)

- Initial version




