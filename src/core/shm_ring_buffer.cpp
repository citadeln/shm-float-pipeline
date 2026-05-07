#include "../include/shm_ring_buffer.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <iostream>
#include <string_view>
#include <string>

namespace shm {

constexpr mode_t kShmPermissions =
    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;  // 0666
constexpr mode_t kSemPermissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

// Вспомогательный вывод ошибки
void print_err(const char* where, int err = errno) {
  std::cerr << "Error in " << where << ": " << strerror(err) << " (" << err << ")\n";
}

RingBuffer::RingBuffer() noexcept = default;
RingBuffer::~RingBuffer() { Cleanup(); }

bool RingBuffer::InitProducer() noexcept {
  if (fd_ != -1 || addr_ || header_ || data_) {
    return false;
  }

  // Создаём /test_channel с O_CREAT | O_RDWR, права 0666
  fd_ = shm_open("/test_channel", O_CREAT | O_RDWR, kShmPermissions);
  if (fd_ < 0) {
    print_err("Producer shm_open");
    return false;
  }

  if (ftruncate(fd_, kShmSize) < 0) {
    print_err("Producer ftruncate");
    Cleanup();
    return false;
  }

  addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (addr_ == MAP_FAILED) {
    print_err("Producer mmap");
    Cleanup();
    return false;
  }

  header_ = static_cast<Header*>(addr_);
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;
  header_->capacity = (kShmSize - kDataOffset) / kItemSize;
  header_->magic = 0xDEADBEEF;

  // Создаём именованные семафоры empty / full
  sem_empty_ = sem_open("/test_empty", O_CREAT, kSemPermissions, header_->capacity);
  if (sem_empty_ == SEM_FAILED) {
    print_err("Producer sem_open /test_empty");
    Cleanup();
    return false;
  }

  sem_full_ = sem_open("/test_full", O_CREAT, kSemPermissions, 0);
  if (sem_full_ == SEM_FAILED) {
    print_err("Producer sem_open /test_full");
    Cleanup();
    return false;
  }

  return true;
}

bool RingBuffer::InitConsumer() noexcept {
  if (fd_ != -1 || addr_ || header_ || data_) {
    return false;
  }

  // Открываем существующий объект (O_RDONLY, но всё равно нужен `mode`)
  fd_ = shm_open("/test_channel", O_RDWR, kShmPermissions);
  if (fd_ < 0) {
    print_err("Consumer shm_open /test_channel");
    return false;
  }

  // Consumer тоже делает `PROT_READ | PROT_WRITE`, т.к. mmap для MAP_SHARED
  addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (addr_ == MAP_FAILED) {
    print_err("Consumer mmap");
    Cleanup();
    return false;
  }

  header_ = static_cast<Header*>(addr_);
  if (header_->magic != 0xDEADBEEF) {
    std::cerr << "Consumer Error: header magic mismatch\n";
    Cleanup();
    return false;
  }
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;

  // Consumer только открывает существующие именованные семафоры
  sem_empty_ = sem_open("/test_empty", 0);  // 0 = O_RDONLY для семафора
  if (sem_empty_ == SEM_FAILED) {
    print_err("Consumer sem_open /test_empty");
    Cleanup();
    return false;
  }

  sem_full_ = sem_open("/test_full", 0);  // 0 = O_RDONLY
  if (sem_full_ == SEM_FAILED) {
    print_err("Consumer sem_open /test_full");
    Cleanup();
    return false;
  }

  return true;
}

bool RingBuffer::Push(std::span<const std::uint8_t> item) noexcept {
  if (!addr_ || !header_ || !data_ || item.size() > kItemSize) {
    return false;
  }

  if (sem_wait(static_cast<sem_t*>(sem_empty_)) != 0) {
    return false;
  }
  std::memcpy(data_ + (header_->head * kItemSize), item.data(), item.size());
  header_->head = (header_->head + 1) % header_->capacity;
  if (sem_post(static_cast<sem_t*>(sem_full_)) != 0) {
    // хотя бы данные записаны
  }
  return true;
}

bool RingBuffer::Pop(std::span<std::uint8_t> item) noexcept {
  if (!addr_ || !header_ || !data_ || item.size() < kItemSize) {
    return false;
  }

  if (sem_wait(static_cast<sem_t*>(sem_full_)) != 0) {
    return false;
  }
  std::memcpy(item.data(), data_ + (header_->tail * kItemSize), kItemSize);
  header_->tail = (header_->tail + 1) % header_->capacity;
  if (sem_post(static_cast<sem_t*>(sem_empty_)) != 0) {
    // хотя бы данные вычитаны
  }
  return true;
}

// void RingBuffer::Cleanup() noexcept {
//   if (addr_) {
//     munmap(addr_, kShmSize);
//     addr_ = nullptr;
//     header_ = nullptr;
//     data_ = nullptr;
//   }

//   if (fd_ >= 0) {
//     close(fd_);
//     fd_ = -1;
//   }

//   if (sem_empty_) {
//     sem_close(static_cast<sem_t*>(sem_empty_));
//     sem_unlink("/test_empty");
//     sem_empty_ = nullptr;
//   }

//   if (sem_full_) {
//     sem_close(static_cast<sem_t*>(sem_full_));
//     sem_unlink("/test_full");
//     sem_full_ = nullptr;
//   }

//   // Удаляем shm объект только при завершении работы Producer/Consumer
//   shm_unlink("/test_channel");
// }

void RingBuffer::Cleanup() noexcept {
  if (addr_) {
    munmap(addr_, kShmSize);
    addr_ = nullptr;
    header_ = nullptr;
    data_ = nullptr;
  }

  if (fd_ >= 0) {
    close(fd_);
    fd_ = -1;
  }

  if (sem_empty_) {
    sem_close(static_cast<sem_t*>(sem_empty_));
    sem_empty_ = nullptr;
  }

  if (sem_full_) {
    sem_close(static_cast<sem_t*>(sem_full_));
    sem_full_ = nullptr;
  }
}

void* RingBuffer::GetFullSemaphore() const noexcept { return sem_full_; }

}  // namespace shm
