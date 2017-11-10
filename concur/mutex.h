#pragma once

#include <atomic>
#include <list>

#include "sched/task.h"

namespace gthread {
struct mutexattr {};

constexpr mutexattr k_default_mutexattr{};

class mutex {
 public:
  mutex(const mutexattr& a);
  ~mutex();

  int lock();

  int unlock();

  // private:
  std::atomic_flag _lock;
  task* _owner;
  std::list<task*> _waitqueue;
  uint64_t _priority_boost;
};
}  // namespace gthread
