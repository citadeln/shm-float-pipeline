#include "metrics.h"

#include <cmath>

namespace metrics {

Timer::Timer() : start_(std::chrono::high_resolution_clock::now()) {
}

void Timer::Start() {
  start_ = std::chrono::high_resolution_clock::now();
}

std::int64_t Timer::ElapsedMillis() const {
  auto now = std::chrono::high_resolution_clock::now();
  auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_);
  return diff.count();
}

double CompressionRatio(std::size_t original_bytes, std::size_t compressed_bytes) {
  if (original_bytes == 0) {
    return 0.0;
  }
  return static_cast<double>(compressed_bytes) /
         static_cast<double>(original_bytes);
}

double LossPercent(const float* original, const float* reconstructed,
                   std::size_t count) {
  if (count == 0) {
    return 0.0;
  }
  std::size_t changed = 0;
  for (std::size_t i = 0; i < count; ++i) {
    if (original[i] != reconstructed[i]) {
      ++changed;
    }
  }
  return 100.0 * static_cast<double>(changed) /
         static_cast<double>(count);
}

}  // namespace metrics
