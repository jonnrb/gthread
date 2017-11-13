#include "sched/task.h"

#include <atomic>

#include "platform/memory.h"
#include "sched/task_attr.h"
#include "util/log.h"

namespace gthread {

task::end_handler task::_end_handler{nullptr};

// task switching MUST not be reentrant
std::atomic_flag task::_lock{false};

bool task::_timer_enabled{false};
task::time_slice_trap task::_time_slice_trap{nullptr};

std::atomic<bool> task::_is_root_task_init{false};
task task::_root_task;

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
  free_stack((char*)stack - total_stack_size, total_stack_size);
}

// root task constructor
task::task()
    : _tls{nullptr},
      _ctx{0},
      _stack{nullptr},
      _stack_begin{nullptr},
      _total_stack_size{0},
      joiner{nullptr},
      run_state{RUNNING},
      entry{nullptr},
      arg{nullptr},
      return_value{nullptr},
      vruntime{0},
      priority_boost{0} {}

task::task(void* stack, void* stack_begin, size_t total_stack_size,
           bool alloc_tls)
    : _ctx{0},
      _stack{stack},
      _stack_begin{stack_begin},
      _total_stack_size{total_stack_size},
      joiner{nullptr},
      run_state{STOPPED},
      entry{nullptr},
      arg{nullptr},
      return_value{nullptr},
      vruntime{0},
      priority_boost{0} {
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
  if (this == &_root_task) return;
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

void task::record_time_slice(std::chrono::microseconds elapsed) {
  // XXX hack to hurt spinlocks
  if (elapsed.count() < 10) elapsed = std::chrono::microseconds{10};

  if (priority_boost > 0) {
    elapsed /= (priority_boost + 1);
  }

  vruntime += elapsed;
}

void task::reset_timer_and_record_time() {
  if (_timer_enabled) {
    auto elapsed = alarm::reset();
    record_time_slice(elapsed);
  }
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

int task::start() {
  if (branch_unexpected(run_state != STOPPED)) {
    return -1;
  }

  auto* cur = current();

  if (branch_unexpected(this == cur)) {
    return -1;
  }

  if (_lock.test_and_set()) return -1;

  gthread_saved_ctx_t* new_and_cur[2] = {&this->_ctx, &cur->_ctx};
  gthread_switch_to_and_spawn(&cur->_ctx, _stack_begin, gthread_task_entry,
                              new_and_cur);
  run_state = SUSPENDED;

  _lock.clear();

  return 0;
}

void task::stop(void* return_value) {
  this->return_value = return_value;
  if (branch_expected(_end_handler)) {
    _end_handler(this);
  }
  gthread_log_fatal("task end handler did not stop the task!");
}

int task::switch_to_internal(std::chrono::microseconds* elapsed) {
  auto prev_task = current();
  if (this == prev_task) {
    gthread_log_fatal("switching to current task bad!");
    return 0;
  }

  // slow path on being the first call in this module
  if (branch_unexpected(!_is_root_task_init)) {
    init();
  }

  if (_lock.test_and_set()) return -1;

  // if the task sets its own `run_state`, respect that value
  if (prev_task->run_state == RUNNING) {
    prev_task->run_state = SUSPENDED;
  }

  if (elapsed == nullptr) {
    prev_task->reset_timer_and_record_time();
  } else {
    prev_task->record_time_slice(*elapsed);
  }

  // officially in the |task|'s context
  if (_tls != nullptr) {
    _tls->set_thread(this);
    _tls->use();
  } else {
    tls::current()->set_thread(this);
  }

  _lock.clear();
  gthread_switch_to(&prev_task->_ctx, &_ctx);

  if (branch_unexpected(prev_task != current())) {
    gthread_log_fatal("task was not switched back to properly");
  }

  prev_task->run_state = RUNNING;
  return 0;
}

int task::switch_to() { return switch_to_internal(nullptr); }

void task::time_slice_trap_base(std::chrono::microseconds elapsed) {
  task* next_task = nullptr;
  task* cur = current();

  if (_time_slice_trap) {
    next_task = _time_slice_trap(cur);
  }

  if (next_task != nullptr && next_task != cur) {
    if (branch_unexpected(next_task->switch_to_internal(&elapsed))) {
      gthread_log_fatal("error switching to next task");
    }
  } else {
    cur->record_time_slice(elapsed);
  }
}

void task::set_end_handler(end_handler handler) { _end_handler = handler; }

void task::init() {
  bool expected = false;
  if (!_is_root_task_init &&
      _is_root_task_init.compare_exchange_strong(expected, true)) {
    // most of struct is zero-initialized
    _root_task._tls = tls::current();
    _root_task.run_state = RUNNING;

    _root_task._tls->set_thread(&_root_task);
  }
}

}  // namespace gthread
