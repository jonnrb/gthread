#pragma once

#include <chrono>

#include "sched/sched_node.h"
#include "util/compiler.h"

namespace gthread {
constexpr auto preempt_interval = std::chrono::milliseconds{50};

/**
 * enables task preemption by a timer for the entire program
 */
void enable_timer_preemption();

/**
 * disables task preemption by a timer for the entire program
 */
void disable_timer_preemption();

/**
 * when held, blocks preemption of the current execution context
 */
class mt_capability("mutex") preempt_mutex {
 public:
  /**
   * returns the mutex that controls the current execution context
   */
  static preempt_mutex& get();

  /**
   * initiates a critical section
   */
  void lock() mt_acquire();

  /**
   * leaves a critical section
   */
  void unlock() mt_release();

  operator bool() const { return task::current()->no_preempt_flag.load(); }

  sched_node& node() const mt_locks_required(this);

 private:
  preempt_mutex() {}
};
}  // namespace gthread
