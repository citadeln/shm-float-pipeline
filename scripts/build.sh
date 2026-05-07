#!/bin/bash
set -e

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../" && pwd)"
cd "$PROJ_ROOT"

echo "Building with Clang 18 + LLD..."
rm -rf build
mkdir build && cd build

cmake -DCMAKE_CXX_COMPILER=clang++ \
      -DCMAKE_CXX_STANDARD=17 \
      -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=ld.lld" \
      ..

make -j$(nproc)

echo "Testing..."
ctest -V || true

echo "SUCCESS! Clang + LLD ready"
echo "./producer ../data/input.bin &"
