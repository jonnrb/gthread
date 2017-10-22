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

namespace gthread {
inline task* task::current() {
  // slow path on being the first call in this module
  if (branch_unexpected(!is_root_task_init)) {
    init();
  }

  return (task*)gthread_tls_current_thread();
}
}  // namespace gthread

#endif  // SCHED_TASK_INLINE_H_
