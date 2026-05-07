#!/bin/bash
set -e

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../" && pwd)"
cd "$PROJ_ROOT"

echo "Building for Debian (Clang + LLD)..."
rm -rf build
mkdir build && cd build

export ld=$(which ld.lld)

cmake -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=20 ..

make -j$(nproc)

echo "SUCCESS!"
