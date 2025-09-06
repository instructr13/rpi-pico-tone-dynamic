# Dynamic PWM Tone for Raspberry Pi Pico

This project enables a single piezo speaker to produce sound while changing its frequency and adjusting its volume.

Although the frequency range is limited, it can also generate waveforms other than square waves.

## Features

- ToneDynamic: An extended version of Arduino `tone()` function that allows dynamic frequency and duty cycle control.
  Fully asynchronous and non-blocking.
- Speaker: A audio class that allows volume control and waveform selection. Partially asynchronous. Uses high-frequency PWM (1MHz) to achieve variable waveforms and volume control.
- Waveform: A collection of waveform generators including sine, triangle, sawtooth, and more.

## Build & Upload

```
pio run -t upload
```

## Examples

### ToneDynamic

```cpp
#include <Arduino.h>

#include <ToneDynamic.h>

constexpr uint16_t SPEAKER_PIN = 15;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting tone at 1kHz, 50% duty cycle");

  toneDynamic(SPEAKER_PIN, 220);
}

void loop() {
  delay(1000);

  Serial.println("Sweeping Frequency: 100Hz -> 5kHz at 50% duty cycle");

  for (uint16_t f = 100; f <= 5000; f += 10) {
    toneDynamicUpdate(SPEAKER_PIN, f);
    Serial.printf("\rFrequency: %04d Hz", f);

    delay(15);
  }

  Serial.println();

  delay(1000);
}
```

### Speaker

```cpp
#include <Arduino.h>

#include <Speaker.h>
#include <Waveform.h>

constexpr uint16_t SPEAKER_PIN = 15;

Speaker speaker(SPEAKER_PIN);

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println("Starting Speaker at 440Hz, 50% volume");

  speaker.setWaveform(SAW_WAVEFORM);
  speaker.play(440, 50);
}

void loop() {
  delay(1000);

  Serial.println("Sweeping Frequency: 100Hz -> 5kHz at 50% volume");

  for (uint16_t f = 100; f <= 5000; f += 10) {
    speaker.setFrequency(f);
    Serial.printf("\rFrequency: %04d Hz", f);

    delay(15);
  }

  Serial.println();

  delay(1000);

  Serial.println("Sweeping Volume: 0% -> 100% at 440Hz");

  for (float v = 0; v <= 1; v += 0.01f) {
    speaker.setVolume(v);
    Serial.printf("\rVolume: %03d %%", static_cast<int>(v * 100));

    delay(30);
  }

  Serial.println();

  delay(1000);
}
```
