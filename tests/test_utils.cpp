#include <gtest/gtest.h>

#include "../src/include/compressor.h"
#include "../src/include/shm_ring_buffer.h"

// NB: packet::kMaxFloatPerChunk, packet::kHeaderSize, packet::kItemSize
// доступны только если #include <packet.h> — добавим в test_utils
#include "../src/include/packet.h"

namespace codec {
TEST(CompressorTests, QuantizerEncodeDecode) {
  FloatQuantizer quantizer;

  float input[] = {1.0f, 2.0f, 3.0f, 0.0f, -1.0f, -2.0f};
  std::vector<std::int16_t> encoded(6);
  std::vector<float> decoded(6);

  quantizer.Encode(std::span<const float>{input, 6},
                   std::span<std::int16_t>{encoded});
  quantizer.Decode(std::span<const std::int16_t>{encoded},
                   std::span<float>{decoded});

  EXPECT_EQ(decoded.size(), 6);
  for (size_t i = 0; i < 6; ++i) {
    EXPECT_NEAR(decoded[i], input[i], 0.01);
  }
}

TEST(CompressorTests, QuantizerClipping) {
  FloatQuantizer quantizer(1000.0f);  // дефолтный

  const float max_val = 32.767f;
  const float min_val = -32.768f;
  const float overflow_val = 100.0f;
  const float underflow_val = -100.0f;

  float input[] = {max_val, min_val, overflow_val, underflow_val};
  std::vector<std::int16_t> encoded(4);
  std::vector<float> decoded(4);

  quantizer.Encode(std::span<const float>{input},
                   std::span<std::int16_t>{encoded});
  quantizer.Decode(std::span<const std::int16_t>{encoded},
                   std::span<float>{decoded});

  EXPECT_NEAR(decoded[0], max_val, 0.01);
  EXPECT_NEAR(decoded[1], min_val, 0.01);
  EXPECT_NEAR(decoded[2], max_val, 0.01);
  EXPECT_NEAR(decoded[3], min_val, 0.01);
}
}  // namespace codec

namespace shm {
TEST(RingBufferTests, BasicOperations) {
  RingBuffer ring;
  ASSERT_TRUE(ring.InitProducer());

  const std::uint8_t raw_data[] = {0x01, 0x02, 0x03, 0x04};
  std::span<const std::uint8_t> data{raw_data, sizeof(raw_data)};

  // Проверяем результат Push
  bool push_result = ring.Push(data);
  EXPECT_TRUE(push_result) << "Push в буфер должен быть успешным";

  std::vector<std::uint8_t> retrieved_vec(packet::kItemSize);
  std::span<std::uint8_t> retrieved{retrieved_vec};

  // Проверяем результат Pop
  bool pop_result = ring.Pop(retrieved);
  EXPECT_TRUE(pop_result) << "Pop из буфера должен быть успешным";

  // Сравниваем содержимое поэлементно
  EXPECT_EQ(retrieved.size(), data.size());
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(retrieved[i], data[i]) << "Элемент " << i << " не совпадает";
  }
}
}  // namespace shm
