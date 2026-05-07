#include <cstring>
#include <fstream>
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
  if (!ring.InitConsumer()) return 1;

  codec::FloatQuantizer quantizer;
  std::unordered_map<std::uint32_t, std::vector<float>> reassembly;
  std::vector<std::int16_t> encoded(packet::kMaxFloatPerChunk);
  alignas(16) char item[packet::kItemSize];
  packet::ChunkHeader hdr;

  while (true) {
    ring.Pop({reinterpret_cast<std::uint8_t*>(item), packet::kItemSize});
    memcpy(&hdr, item, packet::kHeaderSize);

    if (hdr.eof_flag == 0xFFFF) break;

    std::span<std::int16_t> payload{
        reinterpret_cast<std::int16_t*>(item + packet::kHeaderSize),
        static_cast<std::size_t>(hdr.payload_len / 2)};

    std::vector<float> decoded(payload.size());
    quantizer.Decode(payload, {decoded.data(), decoded.size()});

    auto& msg = reassembly[hdr.msg_id];
    msg.resize(hdr.total_chunks * packet::kMaxFloatPerChunk);
    std::copy(decoded.begin(), decoded.end(),
              msg.begin() + hdr.chunk_seq * packet::kMaxFloatPerChunk);
  }

  std::vector<float> output;
  for (auto& [id, chunks] : reassembly) {
    output.insert(output.end(), chunks.begin(), chunks.end());
  }

  WriteFloatFile(argv[2], output);
  std::cout << "Consumer: " << timer.ElapsedMillis() << "ms, loss: "
            << metrics::LossPercent(original.data(), output.data(),
                                    original.size())
            << "%\n";

  ring.Cleanup();
  return 0;
}
