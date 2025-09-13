#include "../include/ToneDynamic/Speaker.h"

#include <Arduino.h>

constexpr uint16_t PIN_SPEAKER = 8;

void wait_for_serial() {
  static bool led_flag = false;

  while (!Serial) {
    delay(150);
    digitalWrite(LED_BUILTIN, led_flag ? HIGH : LOW);

    led_flag = !led_flag;
  }

  digitalWrite(LED_BUILTIN, LOW);
}

tone_dynamic::Speaker sp{PIN_SPEAKER};

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_SPEAKER, OUTPUT);

  Serial.begin(115200);

  wait_for_serial();

  Serial.println("PWM Dynamic Tone Generator");
  Serial.printf("Build Date: %s %s\n", __DATE__, __TIME__);

  Serial.println("Starting tone at 440Hz, 100% vol");

  sp.set_waveform(tone_dynamic::SAW_WAVEFORM);
  sp.set_frequency(110);
  sp.set_volume(1.0f);
  sp.play();
}

void loop() {
  delay(1000);

  Serial.println("Sweeping Volume: 100% -> 1%");

  for (float v = 1.0f; v >= 0.01f; v -= 0.005f) {
    sp.set_volume(v);

    Serial.printf("\rVolume: %3.2f %%", v * 100.f);
    delay(25);
  }

  Serial.println();
  delay(500);

  Serial.println("Sweeping Volume: 1% -> 100%");

  for (float v = 0.01f; v <= 1.0f; v += 0.005f) {
    sp.set_volume(v);

    Serial.printf("\rVolume: %3.2f %%", v * 100.f);
    delay(25);
  }

  Serial.println();
  delay(1000);

  Serial.println("Sweeping Frequency: 100Hz -> 2kHz");

  for (uint16_t f = 100; f <= 5000; f += 10) {
    sp.set_frequency(f);

    Serial.printf("\rFrequency: %04d Hz", f);
    delay(15);
  }

  Serial.println();

  delay(1000);
}
