#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>

namespace codec {

// границы int16_t в виде float
constexpr float kDefaultScale =
    1000.0f;  // масштаб для квантования float в int16
constexpr float kInt16MaxF =
    static_cast<float>(std::numeric_limits<std::int16_t>::max());  // 32767.0f
constexpr float kInt16MinF =
    static_cast<float>(std::numeric_limits<std::int16_t>::min());  // -32768.0f
constexpr float kInt16RangeF = kInt16MaxF - kInt16MinF;

constexpr std::int16_t kInt16Max = std::numeric_limits<std::int16_t>::max();
constexpr std::int16_t kInt16Min = std::numeric_limits<std::int16_t>::min();

class FloatQuantizer {
 public:
  explicit constexpr FloatQuantizer(float scale = kDefaultScale) noexcept
      : scale_(scale) {}

  void Encode(std::span<const float> input,
              std::span<std::int16_t> output) const noexcept;

  void Decode(std::span<const std::int16_t> input,
              std::span<float> output) const noexcept;

 private:
  float scale_;
};

}  // namespace codec
