/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/task_inline.h
 * info: generic task switching functions for a scheduler
 */

#ifndef SCHED_TASK_INLINE_H_
#define SCHED_TASK_INLINE_H_

#include <atomic>

#include "platform/tls.h"
#include "util/compiler.h"

static inline gthread_task_t* gthread_task_current() {
  extern void gthread_task_module_init();
  extern std::atomic<bool> gthread_task_is_root_task_init;

  // slow path on being the first call in this module
  if (branch_unexpected(!gthread_task_is_root_task_init)) {
    gthread_task_module_init();
  }

  return (gthread_task_t*)gthread_tls_current_thread();
}

#endif  // SCHED_TASK_INLINE_H_
