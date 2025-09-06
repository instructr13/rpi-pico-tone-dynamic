# Dynamic PWM Tone for Raspberry Pi Pico

This project enables a single piezo speaker to produce sound while changing its frequency and adjusting its volume.

Although the frequency range is limited, it can also generate waveforms other than square waves.

## Features

- ToneDynamic: An extended version of Arduino `tone()` function that allows dynamic frequency and duty cycle control.
  Fully asynchronous and non-blocking.
- Speaker: A audio class that allows volume control and waveform selection. Partially asynchronous.
- Waveform: A collection of waveform generators including sine, triangle, sawtooth, and more.

## Build & Upload

```
pio run -t upload
```
