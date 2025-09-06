#pragma once

#include "Waveform.h"

#include <pico/time.h>

class Speaker {
public:
  explicit Speaker(uint16_t pin, float freq = 440, float volume = 1);

  [[nodiscard]] float get_frequency() const { return audible_freq; }

  [[nodiscard]] float get_volume() const { return volume; }

  [[nodiscard]] bool playing() const { return is_playing; }

  void set_frequency(float freq);

  void set_volume(float vol);

  void set_waveform(const Waveform &wf);

  void play(uint32_t duration = 0);

  void stop();

private:
  static constexpr float carrier_freq = 1000 * 1000; // 1MHz carrier

  uint16_t pin;
  const Waveform *next_waveform = nullptr;
  const Waveform *waveform = &SQUARE_WAVEFORM;

  float audible_freq = 440;
  float volume = 1;
  float duty_scale = 1;
  uint32_t lut_period_us = 0;

  bool is_playing = false;
  bool freq_changed = false;
  uint16_t waveform_index = 0;

  repeating_timer timer{};
  alarm_id_t alarm_id = 0;

  void refresh_lut_period();

  void repeating_timer_cb(repeating_timer *t);
};
