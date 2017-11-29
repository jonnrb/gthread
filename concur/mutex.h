#pragma once

#include <list>

#include "concur/internal/waiter.h"
#include "concur/internal/spin_lock.h"

namespace gthread {
struct mutexattr {};

constexpr mutexattr k_default_mutexattr{};

class mutex {
 public:
  mutex(const mutexattr& a = k_default_mutexattr);
  ~mutex();

  int lock();

  int unlock();

 private:
  internal::spin_lock _lock;
  task* _owner;
  std::list<internal::waiter> _waitqueue;
  uint64_t _priority_boost;
};
}  // namespace gthread
