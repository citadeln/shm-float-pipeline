#include <algorithm>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "../include/compressor.h"
#include "../include/metrics.h"
#include "../include/packet.h"
#include "../include/shm_ring_buffer.h"

// Запись float-файлов
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
  if (argc < 3) {
    std::cerr << "Usage: consumer <original.bin> <output.bin>\n";
    return 1;
  }

  // Чтение исходного файла для сравнения (вычисления loss)
  std::vector<float> original;
  {
    std::ifstream file(argv[1], std::ios::binary);
    if (!file) {
      std::cerr << "Error opening file: " << argv[1] << std::endl;
      return 1;
    }
    float value;
    while (file.read(reinterpret_cast<char*>(&value), sizeof(float))) {
      original.push_back(value);
    }
  }

  metrics::Timer timer;
  shm::RingBuffer ring;

  if (!ring.InitConsumer()) {
    std::cerr << "Consumer Error: Failed to initialize shared memory.\n";
    return 1;
  }

  codec::FloatQuantizer quantizer;

  std::vector<float> output;

  std::unordered_map<std::uint32_t, std::vector<float>> reassembly;

  alignas(16) char item[packet::kItemSize];
  packet::ChunkHeader hdr;

  bool success = true;

  while (true) {
    // Проверка результата Pop
    if (!ring.Pop(std::span<std::uint8_t>{reinterpret_cast<std::uint8_t*>(item),
                                          packet::kItemSize})) {
      std::cerr << "Consumer Error: Failed to pop data from buffer!\n";
      success = false;
      break;
    }

    memcpy(&hdr, item, packet::kHeaderSize);

    if (hdr.eof_flag == 0xFFFF) break;

    // Создаем view (взгляд) на сжатые данные в памяти как на массив int16_t
    std::span<const std::int16_t> payload{
        reinterpret_cast<const std::int16_t*>(item + packet::kHeaderSize),
        static_cast<std::size_t>(hdr.payload_len / 2)};

    // Создаем вектор для декодированных данных и декодируем их
    std::vector<float> decoded(payload.size());
    quantizer.Decode(payload, std::span<float>{decoded});

    auto& chunks = reassembly[hdr.msg_id];
    chunks.resize(hdr.total_chunks * packet::kMaxFloatPerChunk);

    std::copy(decoded.begin(), decoded.end(),
              chunks.begin() + hdr.chunk_seq * packet::kMaxFloatPerChunk);
  }

  // Собираем финальный вектор output из карты reassembly
  for (const auto& [id, chunks] : reassembly) {
    output.insert(output.end(), chunks.begin(), chunks.end());
  }

  // Запись файла теперь использует исправленный вектор 'output'
  WriteFloatFile(argv[2], output);

  double loss_percent =
      metrics::LossPercent(original.data(), output.data(), original.size());

  std::cout << "Consumer: " << timer.ElapsedMillis()
            << "ms, loss: " << std::fixed << std::setprecision(2)
            << loss_percent << "%\n";

  if (success) {
    ring.Cleanup();
  }

  return success ? 0 : 1;
}
