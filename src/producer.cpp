#include <fstream>  // Добавлено для работы с файлами
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

// Чтение float-файлов
inline bool WriteFloatFile(const std::string& filename,
                           const std::vector<float>& data) {
  std::ofstream file(filename, std::ios::binary);
  if (!file) {
    std::cerr << "Error creating file: " << filename << std::endl;
    return false;
  }

  for (float value : data) {
    file.write(reinterpret_cast<const char*>(&value), sizeof(float));
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

  metrics::Metrics metrics;
  metrics.Start();

  shm::ShmRingBuffer ring;
  if (!ring.InitProducer()) {
    return 1;
  }

  // Producer логика: chunking + compress
  std::uint32_t msg_id = 0;
  std::uint32_t total_chunks = (input.size() + packet::kMaxFloatPerChunk - 1) /
                               packet::kMaxFloatPerChunk;
  codec::FloatQuantizer quantizer;

  alignas(16) char item[256];
  std::vector<std::int16_t> encoded(packet::kMaxFloatPerChunk);
  std::size_t orig_bytes = input.size() * 4;
  std::size_t comp_bytes = 0;

  for (std::uint32_t seq = 0; seq < total_chunks; ++seq) {
    auto count = std::min(packet::kMaxFloatPerChunk,
                          input.size() - seq * packet::kMaxFloatPerChunk);
    quantizer.Encode({input.data() + seq * packet::kMaxFloatPerChunk, count},
                     encoded);

    packet::ChunkHeader hdr{msg_id, seq, total_chunks, count * 2};
    std::memcpy(item, &hdr, packet::kHeaderSize);
    std::memcpy(item + packet::kHeaderSize, encoded.data(), count * 2);

    comp_bytes += packet::kHeaderSize + count * 2;
    ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kItemSize});

    // Завершаем передачу пустым пакетом
    if (seq == total_chunks - 1) {
      packet::ChunkHeader eof{0, 0, 0, 0, 0xFFFF};
      std::memcpy(item, &eof, packet::kHeaderSize);
      ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kHeaderSize});
    }
  }

  auto ms = metrics.ElapsedMs();
  std::cout << "Producer: " << ms
            << "ms, ratio: " << metrics.CompressionRatio(orig_bytes, comp_bytes)
            << "\n";

  ring.Cleanup();
  return 0;
}