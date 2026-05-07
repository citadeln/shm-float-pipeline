#ifndef PROJECT_METRICS_H_
#define PROJECT_METRICS_H_

#include <chrono>
#include <cstddef>
#include <cstdint>

namespace metrics {

class Timer {
 public:
  Timer();

  void Start();
  std::int64_t ElapsedMillis() const;

 private:
  std::chrono::high_resolution_clock::time_point start_;
};

double CompressionRatio(std::size_t original_bytes,
                        std::size_t compressed_bytes);

double LossPercent(const float* original, const float* reconstructed,
                   std::size_t count);

}  // namespace metrics

#endif  // PROJECT_METRICS_H_
