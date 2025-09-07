#include "Speaker.h"

#include "ToneDynamic.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float V_PEAK = 3.3f; // V

float calculate_duty_scale_for_volume_basic(float volume) {
  volume = std::clamp(volume, 0.0f, 1.0f);

  if (volume <= 0.01f)
    return 0.0f;

  const auto attenuation_db = 20.0f * std::log10(volume);
  const auto linear_ratio = std::pow(10.0f, attenuation_db / 20.0f);

  const auto v_rms_max = V_PEAK * std::sqrt(0.5f);
  const auto v_rms_target = v_rms_max * linear_ratio;
  const auto duty_cycle = (v_rms_target * v_rms_target) / (V_PEAK * V_PEAK);

  return duty_cycle * 2; // Scale to [0..1]
}

} // namespace

Speaker::Speaker(const uint16_t pin, const bool use_core1, const float freq,
                 const float volume)
    : pin(pin), audible_freq(freq) {
  if (use_core1) {
    alarm_pool = alarm_pool_create(1, 4);
  } else {
    alarm_pool = alarm_pool_get_default();
  }

  refresh_lut_period();
  set_volume(volume);
}

void Speaker::set_frequency(const float freq) {
  audible_freq = freq;

  refresh_lut_period();

  if (!is_playing)
    return;

  freq_changed = true;
}

void Speaker::set_volume(const float vol) {
  volume = std::clamp(vol, 0.f, 1.f);
  duty_scale = calculate_duty_scale_for_volume_basic(volume);
}

void Speaker::set_waveform(const Waveform &wf) {
  if (&wf == next_waveform)
    return;

  if (!is_playing) {
    waveform = &wf;

    refresh_lut_period();

    return;
  }

  next_waveform = &wf;
}

void Speaker::play(const uint32_t duration) {
  if (is_playing)
    return;

  toneDynamic(pin, carrier_freq, 0, 0);

  is_playing = true;

  alarm_pool_add_repeating_timer_us(
      alarm_pool, -static_cast<int64_t>(lut_period_us),
      [](repeating_timer *t) {
        static_cast<Speaker *>(t->user_data)->repeating_timer_cb(t);

        return true;
      },
      this, &timer);

  if (duration > 0) {
    alarm_id = alarm_pool_add_alarm_in_ms(
        alarm_pool, duration,
        [](const alarm_id_t id, void *user_data) {
          (void)id;

          static_cast<Speaker *>(user_data)->stop();

          return static_cast<int64_t>(0);
        },
        this, true);
  }
}

void Speaker::stop() {
  if (!is_playing)
    return;

  is_playing = false;

  cancel_repeating_timer(&timer);

  if (alarm_id) {
    alarm_pool_cancel_alarm(alarm_pool, alarm_id);

    alarm_id = 0;
  }

  noToneDynamic(pin);
}

void Speaker::refresh_lut_period() {
  lut_period_us =
      std::lround(1000 * 1000 / (waveform->get_size() * audible_freq));
}

void Speaker::repeating_timer_cb(repeating_timer *t) {
  if (!is_playing)
    return;

  if (waveform_index == 0 && next_waveform) {
    waveform = next_waveform;
    next_waveform = nullptr;

    lut_period_us =
        std::lround(1000 * 1000 / (waveform->get_size() * audible_freq));

    freq_changed = true;
  }

  if (freq_changed) {
    t->delay_us = -static_cast<int64_t>(lut_period_us);

    freq_changed = false;
  }

  float duty_cycle =
      static_cast<float>(waveform->operator[](waveform_index)) / 255.f;
  duty_cycle = std::clamp(duty_cycle, 0.f, 1.f);

  waveform_index = (waveform_index + 1) & (waveform->get_size() - 1);

  toneDynamicUpdate(pin, carrier_freq, duty_cycle * duty_scale);
}
