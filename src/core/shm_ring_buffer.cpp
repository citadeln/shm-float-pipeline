#include "../include/shm_ring_buffer.h"

#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

namespace shm {

RingBuffer::~RingBuffer() { Cleanup(); }

bool RingBuffer::InitProducer() {
  fd_ = shm_open("/test_channel", O_CREAT | O_RDWR, 0666);
  if (fd_ < 0) return false;

  ftruncate(fd_, kShmSize);
  addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (addr_ == MAP_FAILED) return false;

  header_ = static_cast<Header*>(addr_);
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;
  header_->capacity = (kShmSize - kDataOffset) / kItemSize;

  sem_empty_ = sem_open("/test_empty", O_CREAT, 0666, header_->capacity);
  sem_full_ = sem_open("/test_full", O_CREAT, 0666, 0);
  return sem_empty_ && sem_full_;
}

bool RingBuffer::InitConsumer() {
  fd_ = shm_open("/test_channel", O_RDONLY, 0666);
  if (fd_ < 0) return false;

  addr_ = mmap(nullptr, kShmSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0);
  if (addr_ == MAP_FAILED) return false;

  header_ = static_cast<Header*>(addr_);
  data_ = static_cast<std::uint8_t*>(addr_) + kDataOffset;

  sem_empty_ = sem_open("/test_empty", 0, 0666, 0);
  sem_full_ = sem_open("/test_full", 0, 0666, 0);
  return sem_empty_ && sem_full_;
}

bool RingBuffer::Push(std::span<const std::uint8_t> item) {
  if (item.size() > kItemSize) return false;

  sem_wait(static_cast<sem_t*>(sem_empty_));
  std::memcpy(data_ + (header_->head * kItemSize), item.data(), item.size());
  header_->head = (header_->head + 1) % header_->capacity;
  sem_post(static_cast<sem_t*>(sem_full_));
  return true;
}

bool RingBuffer::Pop(std::span<std::uint8_t> item) {
  if (item.size() < kItemSize) return false;

  sem_wait(static_cast<sem_t*>(sem_full_));
  std::memcpy(item.data(), data_ + (header_->tail * kItemSize), kItemSize);
  header_->tail = (header_->tail + 1) % header_->capacity;
  sem_post(static_cast<sem_t*>(sem_empty_));
  return true;
}

void RingBuffer::Cleanup() {
  if (addr_) munmap(addr_, kShmSize);
  if (fd_ >= 0) close(fd_);
  if (sem_empty_) {
    sem_close(static_cast<sem_t*>(sem_empty_));
    sem_unlink("/test_empty");
  }
  if (sem_full_) {
    sem_close(static_cast<sem_t*>(sem_full_));
    sem_unlink("/test_full");
  }
  shm_unlink("/test_channel");
}

}  // namespace shm
