#include "ToneDynamic.h"

#include <Arduino.h>

#include <algorithm>
#include <map>
#include <optional>

#include "tone_dynamic.pio.h"

namespace {

// Fixed: pull, mov, out, mov, set, set, mov, mov (8 instructions)
//        + 1 cycle at jmp end = 9 cycles
// Variable: high_loop (X+1 cycles) + low_loop (X + 1 cycles)
constexpr int PIO_INSTRUCTION_OVERHEAD = 9;
constexpr int PIO_MIN_PHASE_CYCLES = 1;

constexpr uint32_t MAX_SINGLE_PHASE = 0xFFFF;
constexpr uint32_t MAX_REPRESENTABLE_CYCLES =
    PIO_INSTRUCTION_OVERHEAD + MAX_SINGLE_PHASE + PIO_MIN_PHASE_CYCLES;

struct Tone {
  pin_size_t pin;
  PIO pio;
  int sm;
  int off;
  float current_clkdiv;
  alarm_id_t alarm;
};

struct FrequencyConfig {
  uint32_t packed_value;
  float clkdiv;
};

auto_init_mutex(_toneMutex);

PIOProgram _toneDynamicPgm(&tone_dynamic_program);
std::map<pin_size_t, Tone *> _toneMap;

bool pio_sm_get_enabled(PIO pio, uint sm) {
  check_pio_param(pio);
  check_sm_param(sm);

  return (pio->ctrl & ~(1u << sm)) & (1u << sm);
}

int64_t _stopTonePIO(const alarm_id_t id, void *user_data) {
  (void)id;

  const auto tone = static_cast<Tone *>(user_data);

  tone->alarm = 0;

  digitalWrite(tone->pin, LOW);
  pinMode(tone->pin, OUTPUT);

  pio_sm_set_enabled(tone->pio, tone->sm, false);

  return 0;
}

std::optional<FrequencyConfig> make_packed_value(const float frequency,
                                                 float duty_cycle) {
  if (frequency <= 0)
    return std::nullopt;

  duty_cycle = std::clamp(duty_cycle, 0.f, 1.f);

  const uint32_t sys_clk = RP2040::f_cpu();

  const uint64_t required_cycles64 =
      (sys_clk + frequency / 2) / frequency; // 四捨五入

  const auto required_cycles = static_cast<uint32_t>(required_cycles64);
  float clkdiv = 1;

  if (required_cycles > MAX_REPRESENTABLE_CYCLES) {
    clkdiv = static_cast<float>(required_cycles) / MAX_REPRESENTABLE_CYCLES;

    if (clkdiv > 65536) {
      // Frequency too low
      return std::nullopt;
    }
  }

  const auto effective_clk = static_cast<uint64_t>(sys_clk / clkdiv);

  const uint64_t total_cycles64 =
      (effective_clk + frequency / 2) / frequency; // 四捨五入

  const auto total_cycles = static_cast<uint32_t>(total_cycles64);

  if (total_cycles <= PIO_INSTRUCTION_OVERHEAD) {
    // Frequency too high
    return std::nullopt;
  }

  const uint32_t variable_cycles =
      total_cycles - PIO_INSTRUCTION_OVERHEAD; // = (high_phase + low_phase)

  constexpr uint32_t min_phase = PIO_MIN_PHASE_CYCLES;

  if (variable_cycles < min_phase * 2) {
    // Insufficient cycles for stable operation
    return std::nullopt;
  }

  const uint32_t high_phase =
      std::clamp(static_cast<uint32_t>(lround(variable_cycles * duty_cycle)),
                 min_phase, variable_cycles - min_phase);

  const uint32_t low_phase = variable_cycles - high_phase;

  const uint32_t high_val = high_phase - PIO_MIN_PHASE_CYCLES;
  const uint32_t low_val = low_phase - PIO_MIN_PHASE_CYCLES;

  if (high_val > 0xFFFF || low_val > 0xFFFF) {
    // Should not happen due to previous checks
    return std::nullopt;
  }

  return FrequencyConfig{
      .packed_value = (static_cast<uint32_t>(high_val) << 16) |
                      static_cast<uint32_t>(low_val),
      .clkdiv = clkdiv,
  };
}

} // namespace

void toneDynamic(const uint8_t pin, const float frequency,
                 const uint32_t duration, const float dutyCycle) {
  if (pin >= __GPIOCNT)
    return;

  if (frequency <= 0) {
    noToneDynamic(pin);

    return;
  }

  CoreMutex m(&_toneMutex);

  if (!m)
    return;

  const auto freq_config = make_packed_value(frequency, dutyCycle);

  if (!freq_config)
    return;

  auto entry = _toneMap.find(pin);
  Tone *tone = nullptr;

  if (entry == _toneMap.end()) {
    tone = new Tone{.pin = pin, .current_clkdiv = 1, .alarm = 0};

    pinMode(pin, OUTPUT);

    if (!_toneDynamicPgm.prepare(&tone->pio, &tone->sm, &tone->off, pin, 1)) {
      delete tone;

      return;
    }
  } else {
    tone = entry->second;

    if (tone->alarm) {
      cancel_alarm(tone->alarm);

      tone->alarm = 0;
    }
  }

  if (!pio_sm_get_enabled(tone->pio, tone->sm)) {
    tone_dynamic_program_init(tone->pio, tone->sm, tone->off, pin);

    tone->current_clkdiv = freq_config->clkdiv;

    pio_sm_set_clkdiv(tone->pio, tone->sm, freq_config->clkdiv);
  } else if (tone->current_clkdiv != freq_config->clkdiv) {
    tone->current_clkdiv = freq_config->clkdiv;

    pio_sm_set_clkdiv(tone->pio, tone->sm, freq_config->clkdiv);
  }

  pio_sm_clear_fifos(tone->pio, tone->sm);
  pio_sm_put(tone->pio, tone->sm, freq_config->packed_value);
  pio_sm_set_enabled(tone->pio, tone->sm, true);

  _toneMap[pin] = tone;

  if (duration) {
    const auto ret = add_alarm_in_ms(duration, _stopTonePIO, tone, true);

    if (ret > 0) {
      tone->alarm = ret;
    }
  }
}

void noToneDynamic(const uint8_t pin) {
  CoreMutex m(&_toneMutex);

  if ((pin > __GPIOCNT) || !m)
    return;

  auto entry = _toneMap.find(pin);

  if (entry == _toneMap.end())
    return;

  if (entry->second->alarm) {
    cancel_alarm(entry->second->alarm);

    entry->second->alarm = 0;
  }

  pio_sm_set_enabled(entry->second->pio, entry->second->sm, false);
  pio_sm_unclaim(entry->second->pio, entry->second->sm);

  delete entry->second;

  _toneMap.erase(entry);

  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
}

void toneDynamicUpdate(const uint8_t pin, const float frequency,
                       const float dutyCycle) {
  if (pin >= __GPIOCNT || frequency <= 0)
    return;

  CoreMutex m(&_toneMutex);

  if (!m)
    return;

  const auto entry = _toneMap.find(pin);

  if (entry == _toneMap.end())
    return;

  const auto tone = entry->second;

  const auto freq_config = make_packed_value(frequency, dutyCycle);

  if (!freq_config)
    return;

  if (tone->current_clkdiv != freq_config->clkdiv) {
    tone->current_clkdiv = freq_config->clkdiv;

    pio_sm_set_clkdiv(tone->pio, tone->sm, freq_config->clkdiv);
  }

  pio_sm_clear_fifos(tone->pio, tone->sm);
  pio_sm_put(tone->pio, tone->sm, freq_config->packed_value);
}
