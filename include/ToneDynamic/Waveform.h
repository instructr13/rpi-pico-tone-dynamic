#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <random>

#include <pico/rand.h>

class Waveform {
public:
  using value_type = std::uint8_t;
  using table_type = std::array<value_type, 1 << 8>;

  virtual ~Waveform() = default;

  [[nodiscard]] virtual uint16_t get_size() const = 0;

  [[nodiscard]] virtual value_type operator[](std::size_t idx) const = 0;
};

class DataWaveform final : public Waveform {
public:
  template <std::size_t N>
  explicit DataWaveform(const std::array<value_type, N> &tbl) : size(N) {
    static_assert((N & (N - 1)) == 0, "Size must be a power of 2");
    static_assert(N <= 1 << 8, "Size exceeds maximum table size");

    std::copy_n(tbl.begin(), N, table.begin());
  }

  [[nodiscard]] uint16_t get_size() const override { return size; }

  [[nodiscard]] value_type operator[](const std::size_t idx) const override {
    return table[idx & (size - 1)];
  }

private:
  uint16_t size;
  table_type table{};
};

class NoiseWaveform final : public Waveform {
public:
  NoiseWaveform() : rng(get_rand_32()) {}

  [[nodiscard]] uint16_t get_size() const override { return 1; }

  [[nodiscard]] value_type operator[](std::size_t idx) const override {
    return static_cast<value_type>(rng() & 0xFF);
  }

private:
  mutable std::mt19937 rng;
};

// Generation JavaScript:
// const waveLUT = (N, f) => [...Array(N).keys()].map(i => i / N).map(f).map(v
// => Math.round(127.5 + v * 127.5))

const DataWaveform SQUARE_WAVEFORM{
    std::array<Waveform::value_type, 2>{{255, 0}}};

const DataWaveform SQUARE_25_WAVEFORM{
  std::array<Waveform::value_type, 4>{{255, 0, 0, 0}}};

const DataWaveform SQUARE_12_WAVEFORM{
    std::array<Waveform::value_type, 8>{{255, 0, 0, 0, 0, 0, 0, 0}}};

// waveLUT(16, t => t < 0.25 ? 4 * t : t < 0.75 ? -4 * t + 2 : 4 * t - 4)
const DataWaveform TRIANGLE_WAVEFORM{std::array<Waveform::value_type, 16>{
    {128, 159, 191, 223, 255, 223, 191, 159, 128, 96, 64, 32, 0, 32, 64, 96}}};

// waveLUT(16, t => t => t < 0.5 ? 2 * t : 2 * t - 2)
const DataWaveform SAW_WAVEFORM{std::array<Waveform::value_type, 16>{
    {128, 143, 159, 175, 191, 207, 223, 239, 0, 16, 32, 48, 64, 80, 96, 112}}};

// waveLUT(16, t => Math.sin(2 * Math.PI * t))
const DataWaveform SINE_WAVEFORM{std::array<Waveform::value_type, 16>{
    {128, 176, 218, 245, 255, 245, 218, 176, 128, 79, 37, 10, 0, 10, 37, 79}}};

const NoiseWaveform NOISE_WAVEFORM{};
