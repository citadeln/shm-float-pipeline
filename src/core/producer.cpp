#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <thread>
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
    std::cerr << "Producer: Failed to initialize shared memory.\n";
    return 1;
  }

  // ОЖИДАНИЕ ПОТРЕБИТЕЛЯ
  // Производитель ждет, пока Потребитель откроет семафор /test_full.
  // Это синхронизирует запуск процессов.
  int wait_count = 0;
  while (ring.GetFullSemaphore() == nullptr) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    wait_count += 100;
    if (wait_count > 5000) {  // Таймаут 5 секунд
      std::cerr << "Producer Error: Consumer did not start in time!\n";
      ring.Cleanup();
      return 1;
    }
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
    quantizer.Encode(
        std::span<const float>{input.data() + seq * packet::kMaxFloatPerChunk,
                               count},
        std::span<std::int16_t>{encoded.data(), count});

    packet::ChunkHeader hdr{msg_id, seq, total_chunks,
                            static_cast<uint16_t>(count * 2)};
    memcpy(item, &hdr, packet::kHeaderSize);
    memcpy(item + packet::kHeaderSize, encoded.data(), count * sizeof(int16_t));

    comp_bytes += packet::kHeaderSize + count * sizeof(int16_t);
    ring.Push(std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(item), packet::kItemSize});
  }

  // Отправляем маркер окончания передачи с проверкой результата
  packet::ChunkHeader eof{0, 0, 0, 0, 0xFFFF};
  memcpy(item, &eof, packet::kHeaderSize);

  if (!ring.Push(std::span<const std::uint8_t>{
          reinterpret_cast<const std::uint8_t*>(item), packet::kHeaderSize})) {
    std::cerr << "Producer Error: Failed to push EOF marker!\n";
    ring.Cleanup();
    return 1;
  }

  // Форматированный вывод времени и ratio
  double ratio = metrics::CompressionRatio(orig_bytes, comp_bytes);

  std::cout << "Producer: " << timer.ElapsedMillis() << "ms, ratio: "
            << std::fixed  // Фиксированная точка (не научная нотация)
            << std::setprecision(3)  // Точность до тысячных (0.500)
            << ratio << "\n";

  return 0;
}