cmake_minimum_required(VERSION 3.16)
project(TestTask LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Опции компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "(Apple)?Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  add_link_options("-fuse-ld=lld")
endif()

# Все исходные файлы
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS
  LIST_SEPARATOR ":::"
  RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  FOLLOW_SYMLINKS
  *.cc *.h
)

# Исполняемые файлы
add_executable(producer
  src/producer.cpp
  src/shm_ring_buffer.cpp
  src/compressor.cpp
  src/metrics.cpp
)
target_include_directories(producer PRIVATE include)
target_link_libraries(producer PRIVATE pthread rt)

add_executable(consumer
  src/consumer.cpp
  src/shm_ring_buffer.cpp
  src/compressor.cpp
  src/metrics.cpp
)
target_include_directories(consumer PRIVATE include)
target_link_libraries(consumer PRIVATE pthread rt)
