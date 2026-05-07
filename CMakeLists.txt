cmake_minimum_required(VERSION 3.16)
project(TestTask LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -march=native)
  add_link_options("-fuse-ld=ld.lld")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  add_compile_options(-Wall -Wextra -Wpedantic -march=native)
endif()

add_executable(producer 
  src/core/producer.cpp 
  src/core/shm_ring_buffer.cpp 
  src/core/compressor.cpp 
  src/core/metrics.cpp
)
target_include_directories(producer PRIVATE include)
target_link_libraries(producer PRIVATE pthread rt)

add_executable(consumer 
  src/core/consumer.cpp 
  src/core/shm_ring_buffer.cpp 
  src/core/compressor.cpp 
  src/core/metrics.cpp
)
target_include_directories(consumer PRIVATE include)
target_link_libraries(consumer PRIVATE pthread rt)
