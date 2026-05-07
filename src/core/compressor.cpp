#include "../include/compressor.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace codec {

void FloatQuantizer::Encode(std::span<const float> input,
                            std::span<std::int16_t> output) const {
  assert(input.size() <= output.size());

  for (std::size_t i = 0; i < input.size(); ++i) {
    float scaled_value = input[i] * scale_;
    if (scaled_value > kInt16Max) {
      scaled_value = kInt16Max;
    } else if (scaled_value < kInt16Min) {
      scaled_value = kInt16Min;
    }
    output[i] = static_cast<std::int16_t>(std::round(scaled_value));
  }
}

void FloatQuantizer::Decode(std::span<const std::int16_t> input,
                            std::span<float> output) const {
  assert(input.size() <= output.size());

  const float inv_scale = 1.0f / scale_;
  for (std::size_t i = 0; i < input.size(); ++i) {
    output[i] = static_cast<float>(input[i]) * inv_scale;
  }
}

}  // namespace codec
