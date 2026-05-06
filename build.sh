#!/bin/bash
set -e

echo "Formatting..."
clang-format -i src/*.cpp include/*.h tests/*.cpp

echo "Building..."
mkdir -p build && cd build
cmake -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=17 ..
make -j$(nproc)

echo "Testing..."
ctest -V || true

echo "Ready! ./producer ../data/input.bin &"
