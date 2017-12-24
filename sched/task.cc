#include "sched/task.h"

#include <cassert>

#include "platform/memory.h"
#include "sched/task_attr.h"
#include "util/log.h"

namespace gthread {
task::end_handler task::_end_handler{nullptr};

task* task::create(const attr& a) {
  void* stack;
  size_t total_stack_size;
  allocate_stack(a, &stack, &total_stack_size);

  void* task_place = (void*)((char*)stack - sizeof(task));
  return new (task_place)
      task(stack, task_place, total_stack_size, a.alloc_tls);
}

void task::destroy() {
  void* stack = this->_stack;
  size_t total_stack_size = this->_total_stack_size;
  this->~task();
  if (stack != nullptr) {
    free_stack((char*)stack - total_stack_size, total_stack_size);
  }
}

task::task()
    : _tls{nullptr},
      _ctx{0},
      _stack{nullptr},
      _stack_begin{nullptr},
      _total_stack_size{0},
      run_state{RUNNING},
      entry{nullptr},
      arg{nullptr},
      return_value{nullptr},
      joiner{nullptr},
      detached{false},
      vruntime{0},
      priority_boost{0},
      no_preempt_flag{false} {}

namespace {
static void* sixteen_byte_align(void* p) {
  return reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(p) &
                                 (~((uintptr_t)0xF)));
}
}  // namespace

task::task(void* stack, void* stack_begin, size_t total_stack_size,
           bool alloc_tls)
    : _ctx{0},
      _stack{stack},
      _stack_begin{sixteen_byte_align(stack_begin)},
      _total_stack_size{total_stack_size},
      run_state{STOPPED},
      entry{nullptr},
      arg{nullptr},
      return_value{nullptr},
      joiner{nullptr},
      detached{false},
      vruntime{0},
      priority_boost{0},
      no_preempt_flag{false} {
  if (alloc_tls) {
    void* tls_begin = (void*)((char*)_stack_begin - tls::postfix_bytes());
    _tls = new (tls_begin) tls();
    _stack_begin = (void*)((char*)tls_begin - tls::prefix_bytes());
    _tls->set_thread(this);
  } else {
    _tls = nullptr;
  }
}

task::~task() {
  // only true of wrapped tasks
  if (_stack == nullptr) return;

  if (_tls != nullptr) {
    _tls->~tls();
  }
}

void task::reset() {
  if (_tls) {
    _tls->reset();
  }

  run_state = STOPPED;
  return_value = nullptr;
  joiner = nullptr;
  vruntime = std::chrono::microseconds{0};
  priority_boost = 0;
}

extern "C" {
// root of task stack trace! :)
static void gthread_task_entry(void* arg) {
  // swtich back to the starter to clean up
  gthread_saved_ctx_t** from_and_to = (gthread_saved_ctx_t**)arg;
  gthread_switch_to(from_and_to[0], from_and_to[1]);

  // when we are here, tls should be set up
  auto* cur = task::current();
  cur->run_state = task::RUNNING;
  cur->stop(cur->entry(cur->arg));
}
};

void task::start() {
  assert(branch_unexpected(run_state == STOPPED));

  auto* cur = current();
  assert(this != cur);

  gthread_saved_ctx_t* new_and_cur[2] = {&this->_ctx, &cur->_ctx};
  gthread_switch_to_and_spawn(&cur->_ctx, _stack_begin, gthread_task_entry,
                              new_and_cur);
  run_state = SUSPENDED;
}

void task::stop(void* return_value) {
  this->return_value = return_value;
  if (branch_expected(_end_handler)) {
    _end_handler(this);
  }
  gthread_log_fatal("task end handler did not stop the task!");
}

void task::switch_to() { switch_to_internal<void()>(nullptr); }

void task::set_end_handler(end_handler handler) { _end_handler = handler; }

void task::wrap_current() {
  // most of struct is zero-initialized
  _tls = tls::current();
  run_state = RUNNING;

  _tls->set_thread(this);
}
}  // namespace gthread
