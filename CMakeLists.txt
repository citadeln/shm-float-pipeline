cmake_minimum_required(VERSION 3.16)
project(TestTask CXX)

set(CMAKE_CXX_STANDARD 17)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_link_options("-fuse-ld=lld")
endif()

add_executable(producer src/producer.cpp src/shm_ring.cpp src/compressor.cpp src/metrics.cpp src/utils.cpp)
target_include_directories(producer PRIVATE include)
target_link_libraries(producer pthread rt)

add_executable(consumer src/consumer.cpp src/shm_ring.cpp src/compressor.cpp src/metrics.cpp src/utils.cpp)
target_include_directories(consumer PRIVATE include)
target_link_libraries(consumer pthread rt)

# Clang-format target
add_custom_target(format
  COMMAND clang-format -i ${ALL_SOURCES}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Formatting with clang-format"
)
