cmake_minimum_required(VERSION 3.16)
project(TestTask LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra -Wpedantic -march=native)
add_link_options(-fuse-ld=lld)

# 1. Создаем статическую библиотеку из общего кода (ядра проекта) Это позволяет и
#   producer/consumer, и тестам использовать одну реализацию.
add_library(corelib STATIC src/core/compressor.cpp src/core/metrics.cpp
                           src/core/shm_ring_buffer.cpp)
target_include_directories(corelib PRIVATE include)

# 1. Исполняемый файл Producer
add_executable(producer src/core/producer.cpp)
target_link_libraries(producer PRIVATE corelib pthread rt)

# 1. Исполняемый файл Consumer
add_executable(consumer src/core/consumer.cpp)
target_link_libraries(consumer PRIVATE corelib pthread rt)

# 1. Настройка тестов
find_package(GTest REQUIRED)

add_executable(test_utils tests/test_utils.cpp)
target_link_libraries(test_utils PRIVATE corelib GTest::gtest_main pthread rt)

# Для CI/вручную
add_test(NAME unit_tests COMMAND test_utils)
