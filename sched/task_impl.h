#pragma once

#include <cassert>

#include "platform/alarm.h"
#include "platform/tls.h"
#include "util/compiler.h"

namespace gthread {
inline task* task::current() { return (task*)tls::current_thread(); }

template <typename Thunk>
void task::switch_to_internal(const Thunk* post_tls_switch_thunk) {
  auto* prev_task = current();
  assert(this != prev_task && "switching to current task is a logic error");

  // if the task sets its own `run_state`, respect that value
  if (prev_task->run_state == RUNNING) {
    prev_task->run_state = SUSPENDED;
  }

  // officially in the |task|'s context
  if (_tls != nullptr) {
    _tls->set_thread(this);
    _tls->use();
  } else {
    tls::current()->set_thread(this);
  }

  if (post_tls_switch_thunk != nullptr) {
    (*post_tls_switch_thunk)();
  }

  gthread_switch_to(&prev_task->_ctx, &_ctx);

  assert(prev_task == current() && "switched back to not myself");
  prev_task->run_state = RUNNING;
}

template <typename Thunk>
void task::switch_to(const Thunk& post_tls_switch_thunk) {
  switch_to_internal(&post_tls_switch_thunk);
}
}  // namespace gthread
