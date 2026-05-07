#pragma once
#include <cstdint>

namespace packet {

struct alignas(4) ChunkHeader {
  std::uint32_t msg_id;
  std::uint32_t chunk_seq;
  std::uint32_t total_chunks;
  std::uint16_t payload_len;
  std::uint16_t eof_flag = 0;
};

// размер заголовка (4*4 = 16 байт)
constexpr std::size_t kHeaderSize = 16;

// максимальное число float-элементов в одном куске (переводим в int16)
constexpr std::size_t kMaxFloatPerChunk = 120;

// размер одного элемента в кольце (заголовок + 120 int16)
constexpr std::size_t kItemSize = kHeaderSize + kMaxFloatPerChunk * 2;

}  // namespace packet
