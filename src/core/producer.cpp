#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "../include/compressor.h"
#include "../include/metrics.h"
#include "../include/packet.h"
#include "../include/shm_ring_buffer.h"

// Чтение float-файлов
inline bool ReadFloatFile(const std::string& filename,
                          std::vector<float>* data) {
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error opening file: " << filename << std::endl;
    return false;
  }
  data->clear();
  float value;
  while (file.read(reinterpret_cast<char*>(&value), sizeof(float))) {
    data->push_back(value);
  }
  return true;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: producer <input.bin>\n";
    return 1;
  }

  std::vector<float> input;
  if (!ReadFloatFile(argv[1], &input)) {
    return 1;
  }

  metrics::Timer timer;
  shm::RingBuffer ring;
  if (!ring.InitProducer()) {
    return 1;
  }

  const std::uint32_t msg_id = 0;
  const std::uint32_t total_chunks =
      (input.size() + packet::kMaxFloatPerChunk - 1) /
      packet::kMaxFloatPerChunk;
  codec::FloatQuantizer quantizer;

  alignas(16) char item[packet::kItemSize];
  std::vector<std::int16_t> encoded(packet::kMaxFloatPerChunk);
  const std::size_t orig_bytes = input.size() * sizeof(float);
  std::size_t comp_bytes = 0;

  for (std::uint32_t seq = 0; seq < total_chunks; ++seq) {
    const auto count = std::min(packet::kMaxFloatPerChunk,
                                static_cast<std::uint32_t>(input.size()) -
                                    seq * packet::kMaxFloatPerChunk);
    quantizer.Encode({input.data() + seq * packet::kMaxFloatPerChunk, count},
                     {encoded.data(), count});

    packet::ChunkHeader hdr{msg_id, seq, total_chunks,
                            static_cast<uint16_t>(count * 2)};
    memcpy(item, &hdr, packet::kHeaderSize);
    memcpy(item + packet::kHeaderSize, encoded.data(), count * sizeof(int16_t));

    comp_bytes += packet::kHeaderSize + count * sizeof(int16_t);
    ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kItemSize});
  }

  // Отправляем маркер окончания передачи
  packet::ChunkHeader eof{0, 0, 0, 0, 0xFFFF};
  memcpy(item, &eof, packet::kHeaderSize);
  ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kHeaderSize});

  std::cout << "Producer: " << timer.ElapsedMillis() << "ms, ratio: "
            << metrics::CompressionRatio(orig_bytes, comp_bytes) << "\n";

  ring.Cleanup();
  return 0;
}
