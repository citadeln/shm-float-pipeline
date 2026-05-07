cmake_minimum_required(VERSION 3.16)
project(TestTask LANGUAGES CXX)

# Установка стандарта C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Настройка компилятора и линкера
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -march=native)
  add_link_options("-fuse-ld=ld.lld") # Используем LLD для линковки
endif()

# Сборка исполняемых файлов
add_executable(producer src/core/producer.cpp src/core/shm_ring_buffer.cpp
                        src/core/compressor.cpp src/core/metrics.cpp)
target_link_libraries(producer PRIVATE pthread rt)

add_executable(consumer src/core/consumer.cpp src/core/shm_ring_buffer.cpp
                        src/core/compressor.cpp src/core/metrics.cpp)
target_link_libraries(consumer PRIVATE pthread rt)

# Тесты
enable_testing()
add_subdirectory(tests)
