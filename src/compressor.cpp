#include "compressor.h"

#include <iostream>
#include <string>
#include <vector>

#include "../include/shm_ring_buffer.h"
#include "metrics.h"

namespace {

struct ShmItem {
  std::uint32_t count;
  std::int16_t data[120];
};

}  // namespace
