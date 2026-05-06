#include <iostream>
#include <unordered_map>
#include <vector>

#include "../include/compressor.h"
#include "../include/metrics.h"
#include "../include/packet.h"
#include "../include/shm_ring_buffer.h"

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: consumer <original.bin> <output.bin>\n";
    return 1;
  }

  std::vector<float> original;
  utils::ReadFloatFile(argv[1], &original);

  Metrics metrics;
  metrics.Start();

  ShmRingBuffer ring;
  if (!ring.InitConsumer()) return 1;

  FloatQuantizer quantizer;
  std::unordered_map<std::uint32_t, std::vector<float>> reassembly;
  std::vector<std::int16_t> encoded(packet::kMaxFloatPerChunk);

  alignas(16) char item[256];
  packet::ChunkHeader hdr;

  while (true) {
    ring.Pop({reinterpret_cast<std::uint8_t*>(item), packet::kItemSize});
    std::memcpy(&hdr, item, packet::kHeaderSize);

    if (hdr.eof_flag == 0xFFFF) break;

    std::span<std::int16_t> payload{
        reinterpret_cast<std::int16_t*>(item + packet::kHeaderSize),
        hdr.payload_len};
    std::vector<float> decoded(hdr.payload_len);
    quantizer.Decode(payload, decoded);

    auto& msg = reassembly[hdr.msg_id];
    msg.resize(hdr.total_chunks * packet::kMaxFloatPerChunk);
    std::copy(decoded.begin(), decoded.end(),
              msg.begin() + hdr.chunk_seq * packet::kMaxFloatPerChunk);
  }

  // Flatten all messages to output
  std::vector<float> output;
  for (auto& [id, chunks] : reassembly) {
    output.insert(output.end(), chunks.begin(), chunks.end());
  }

  utils::WriteFloatFile(argv[2], output);

  auto ms = metrics.ElapsedMs();
  double loss =
      metrics.LossPercent(original.data(), output.data(), original.size());
  std::cout << "Consumer: " << ms << "ms, loss: " << loss << "%\n";

  ring.Close();
  return 0;
}
