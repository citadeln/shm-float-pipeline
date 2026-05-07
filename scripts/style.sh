#!/bin/bash

# clang-format -n $(find ./../ -name "*.cpp" -o -name "*.h")
# clang-format -i $(find ./../ -name "*.cpp" -o -name "*.h")

set -e

find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -n
find src include tests -name "*.cpp" -o -name "*.h" | xargs clang-format -i

cmake-format -i CMakeLists.txt || echo "cmake-format skip"

echo "Style OK"