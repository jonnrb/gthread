#pragma once

#include <atomic>

#include "gthread.h"

namespace gthread {
namespace internal {
class spin_lock {
 public:
  void lock() {
    while (_lock.test_and_set(std::memory_order_acquire)) {
      self::yield();
    }
  }

  void unlock() {
    _lock.clear(std::memory_order_release);
  }

 private:
  std::atomic_flag _lock;
};
}  // namespace internal
}  // namespace gthread
