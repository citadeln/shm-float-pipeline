#include <iostream>
#include <string>
#include <vector>

#include "compressor.h"
#include "metrics.h"
#include "shared_memory.h"
#include "utils.h"

namespace {

struct ShmItem {
  std::uint32_t count;
  std::int16_t data[120];
};

}  // namespace

int main(int argc, char** argv) {

}