#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace shm {

struct alignas(4) Header {
  std::uint32_t head = 0;
  std::uint32_t tail = 0;
  std::uint32_t capacity = 0;
  std::uint32_t magic = 0xDEADBEEF;
};

constexpr size_t kShmSize = 256;
constexpr size_t kDataOffset = sizeof(Header);
constexpr size_t kItemSize = 64;  // ChunkHeader + 24 int16

class RingBuffer {
 public:
  RingBuffer() = default;
  ~RingBuffer();

  bool InitProducer();
  bool InitConsumer();
  bool Push(std::span<const std::uint8_t>);
  bool Pop(std::span<std::uint8_t>);
  void Cleanup();

 private:
  int fd_ = -1;
  void* addr_ = nullptr;
  Header* header_ = nullptr;
  std::uint8_t* data_ = nullptr;

  void* sem_empty_ = nullptr;
  void* sem_full_ = nullptr;
};
}  // namespace shm