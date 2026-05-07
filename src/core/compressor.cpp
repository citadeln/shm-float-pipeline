#include "../include/compressor.h"

#include <algorithm>
#include <cmath>

namespace codec {

void FloatQuantizer::Encode(std::span<const float> input,
                            std::span<std::int16_t> output) const noexcept {
  const size_t n = std::min(input.size(), output.size());
  const float scale = scale_;
  const float scaled_max = kInt16MaxF * scale;
  const float scaled_min = kInt16MinF * scale;

  for (size_t i = 0; i < n; ++i) {
    float scaled_value = input[i] * scale;
    if (scaled_value > scaled_max) {
      output[i] = kInt16Max;
    } else if (scaled_value < scaled_min) {
      output[i] = kInt16Min;
    } else {
      output[i] = static_cast<std::int16_t>(std::roundf(scaled_value));
    }
  }
}

void FloatQuantizer::Decode(std::span<const std::int16_t> input,
                            std::span<float> output) const noexcept {
  const size_t n = std::min(input.size(), output.size());
  const float inv_scale = 1.0f / scale_;

  for (size_t i = 0; i < n; ++i) {
    output[i] = static_cast<float>(input[i]) * inv_scale;
  }
}

}  // namespace codec