#!/bin/bash
set -e

OS_TYPE=$(uname -s)

if [[ "$OS_TYPE" == "Linux" ]]; then
  DISTRO=$(lsb_release -si 2>/dev/null || echo "Unknown")
  if [[ "$DISTRO" == "LinuxMint" || "$DISTRO" == "Ubuntu" || "$DISTRO" == "Debian" ]]; then
    echo "Detected Debian-based ($DISTRO). Clang ready."
    COMPILER="clang++"
  else
    echo "Using g++ for $DISTRO"
    COMPILER="g++"
  fi
fi

PROJ_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../" && pwd)"
cd "$PROJ_ROOT"

echo "Building with $COMPILER..."
rm -rf build
mkdir build && cd build

if [[ "$COMPILER" == "clang++" ]]; then
  cmake -DCMAKE_CXX_COMPILER=clang++ \
        -DCMAKE_CXX_STANDARD=17 \
        -DCMAKE_EXE_LINKER_FLAGS="-fuse-ld=ld.lld" \
        ..
else
  cmake -DCMAKE_CXX_COMPILER=g++ -DCMAKE_CXX_STANDARD=17 ..
fi

make -j$(nproc)

echo "SUCCESS!"
echo "./producer ../data/input.bin &"