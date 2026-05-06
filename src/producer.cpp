#include <atomic>
#include <iostream>
#include <span>
#include <vector>

#include "shm_ring.h"
#include "compressor.h"
#include "packet.h"
#include "metrics.h"
#include "utils.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: producer <input.bin>\n";
    return 1;
  }

  std::vector<float> input;
  if (!utils::ReadFloatFile(argv[1], &input)) {
    return 1;
  }

  Metrics metrics;
  metrics.Start();

  ShmRingBuffer ring;
  if (!ring.InitProducer()) {
    return 1;
  }

  // Producer логика: chunking + compress
  std::uint32_t msg_id = 0;
  std::uint32_t total_chunks = (input.size() + packet::kMaxFloatPerChunk - 1) / packet::kMaxFloatPerChunk;
  FloatQuantizer quantizer;
  
  alignas(16) char item[256];
  std::vector<std::int16_t> encoded(packet::kMaxFloatPerChunk);
  std::size_t orig_bytes = input.size() * 4;
  std::size_t comp_bytes = 0;

  for (std::uint32_t seq = 0; seq < total_chunks; ++seq) {
    auto count = std::min(packet::kMaxFloatPerChunk, input.size() - seq * packet::kMaxFloatPerChunk);
    quantizer.Encode({input.data() + seq * packet::kMaxFloatPerChunk, count}, encoded);
    
    packet::ChunkHeader hdr{msg_id++, seq, total_chunks, static_cast<std::uint16_t>(count)};
    std::memcpy(item, &hdr, packet::kHeaderSize);
    std::memcpy(item + packet::kHeaderSize, encoded.data(), count * 2);
    
    ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kItemSize});
    comp_bytes += packet::kItemSize;
  }

  // EOF chunk
  packet::ChunkHeader eof{0, 0, 0, 0, 0xFFFF};
  std::memcpy(item, &eof, packet::kHeaderSize);
  ring.Push({reinterpret_cast<std::uint8_t*>(item), packet::kHeaderSize});

  auto ms = metrics.ElapsedMs();
  std::cout << "Producer: " << ms << "ms, ratio: " 
            << metrics.CompressionRatio(orig_bytes, comp_bytes) << "\n";

  ring.Cleanup();  // shm_unlink + sem_unlink
  return 0;
}
