#include "../include/shm_ring_buffer.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

namespace shm {

constexpr mode_t kShmPermissions =
    S_IRUSR | S_IWUSR | S_IRGRP |
    S_IROTH;  // 0666: владелец rw, группа r, остальные r

RingBuffer::RingBuffer() noexcept = default;

RingBuffer::~RingBuffer() { Cleanup(); }

bool RingBuffer::InitProducer() noexcept {
  if (fd_ != -1 || addr_ || header_ || data_) {
    return false;
  }

  // O_CREAT — создаём сегмент с заданными правами
  fd_ = shm_open("/test_channel", O_CREAT | O_RDWR, kShmPermissions);
  if (fd_ < 0) {
    fd_ = -1;
    return false;
  }

  if (ftruncate(fd_, kShmSize) < 0 ||
      (addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                    0)) == MAP_FAILED) {
    Cleanup();
    return false;
  }

  header_ = static_cast<Header*>(addr_);
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;
  header_->capacity = (kShmSize - kDataOffset) / kItemSize;

  // sem_open — 0666
  constexpr mode_t kSemPermissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  sem_empty_ =
      sem_open("/test_empty", O_CREAT, kSemPermissions, header_->capacity);
  sem_full_ = sem_open("/test_full", O_CREAT, kSemPermissions, 0);

  if (!sem_empty_ || !sem_full_) {
    Cleanup();
    return false;
  }

  return true;
}

bool RingBuffer::InitConsumer() noexcept {
  if (fd_ != -1 || addr_ || header_ || data_) {
    return false;
  }

  fd_ = shm_open("/test_channel", O_RDONLY, 0);
  if (fd_ < 0) {
    fd_ = -1;
    return false;
  }

  if ((addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_,
                    0)) == MAP_FAILED) {
    Cleanup();
    return false;
  }

  header_ = static_cast<Header*>(addr_);
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;

  sem_empty_ = sem_open("/test_empty", 0, 0, 0);
  sem_full_ = sem_open("/test_full", 0, 0, 0);

  if (!sem_empty_ || !sem_full_) {
    Cleanup();
    return false;
  }

  return true;
}

bool RingBuffer::Push(std::span<const std::uint8_t> item) noexcept {
  if (!addr_ || !header_ || !data_ || item.size() > kItemSize) {
    return false;
  }

  sem_wait(static_cast<sem_t*>(sem_empty_));
  std::memcpy(data_ + (header_->head * kItemSize), item.data(), item.size());
  header_->head = (header_->head + 1) % header_->capacity;
  sem_post(static_cast<sem_t*>(sem_full_));
  return true;
}

bool RingBuffer::Pop(std::span<std::uint8_t> item) noexcept {
  if (!addr_ || !header_ || !data_ || item.size() < kItemSize) {
    return false;
  }

  sem_wait(static_cast<sem_t*>(sem_full_));
  std::memcpy(item.data(), data_ + (header_->tail * kItemSize), kItemSize);
  header_->tail = (header_->tail + 1) % header_->capacity;
  sem_post(static_cast<sem_t*>(sem_empty_));
  return true;
}

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
    sem_unlink("/test_empty");
    sem_empty_ = nullptr;
  }

  if (sem_full_) {
    sem_close(static_cast<sem_t*>(sem_full_));
    sem_unlink("/test_full");
    sem_full_ = nullptr;
  }

  shm_unlink("/test_channel");
}

void* RingBuffer::GetFullSemaphore() const noexcept { return sem_full_; }
}  // namespace shm
