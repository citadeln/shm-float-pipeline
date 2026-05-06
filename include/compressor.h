#ifndef COMPRESSED_SHM_CHANNEL_COMPRESSOR_H_
#define COMPRESSED_SHM_CHANNEL_COMPRESSOR_H_

#include <cstddef>
#include <cstdint>
#include <span>

namespace codec {

constexpr float kDefaultScale = 1000.0f;
constexpr std::int16_t kInt16Max = 32767;
constexpr std::int16_t kInt16Min = -32768;

class FloatQuantizer {
 public:
  explicit constexpr FloatQuantizer(float scale = kDefaultScale) : scale_(scale) {}

  void Encode(std::span<const float> input, std::span<std::int16_t> output) const;
  void Decode(std::span<const std::int16_t> input, std::span<float> output) const;

 private:
  float scale_;
};

}  // namespace codec

#endif  // COMPRESSED_SHM_CHANNEL_COMPRESSOR_H_
