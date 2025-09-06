#pragma once

#include <cstdint>

void toneDynamic(uint8_t pin, float frequency, uint32_t duration = 0, float dutyCycle = 0.5);

void noToneDynamic(uint8_t pin);

void toneDynamicUpdate(uint8_t pin, float frequency, float dutyCycle = 0.5);
