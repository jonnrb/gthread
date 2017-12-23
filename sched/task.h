#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include "arch/switch_to.h"
#include "platform/tls.h"
#include "sched/task_attr.h"
#include "util/compiler.h"

// TODO: create task_host

namespace gthread {
struct task {
 public:
  ~task();

  /**
   * factory that constructs a task on that task's own stack
   */
  static task* create(const attr& a);

  /**
   * destructs a task and frees its memory
   */
  void destroy();

  /**
   * creates a task object that represents the current execution context
   *
   * should only be used to wrap an execution context that is already running.
   * this MUST be called from any new context that will call `switch_to()` on a
   * task since `switch_to()` assumes that it can save the current execution
   * context.
   */
  static task create_wrapped();

  void reset();

  /**
   * quickly starts task and switches back to the caller. meant to just get
   * things primed. it is undefined (like reaaallly undefined) as to what
   * happens when a task is switched to that hasn't been `start()`ed.
   */
  void start();

  /**
   * convenience function to call the task's end handler. will assert that
   * the end handler does *not* return. this function will not return.
   */
  void stop(void* return_value);

  /**
   * returns a pointer to the current task
   */
  static task* current();

  /**
   * suspends the currently running task and switches to `this`
   */
  void switch_to();

  /**
   * suspends the currently running task and switches to `this` running
   * |post_tls_switch_thunk| after possibly switching the tls context
   */
  template <typename Thunk>
  void switch_to(const Thunk& post_tls_switch_thunk);

  using end_handler = std::function<void(task*)>;
  static void set_end_handler(end_handler handler);

  constexpr bool has_tls() const { return _tls != nullptr; }

  constexpr size_t stack_size() const { return _total_stack_size; }

 private:
  tls* _tls;

  gthread_saved_ctx_t _ctx;
  void* _stack;
  void* _stack_begin;
  size_t _total_stack_size;

 public:
  typedef enum { RUNNING, SUSPENDED, STOPPED, WAITING } run_state_t;
  run_state_t run_state;

  typedef void* entry_t(void*);
  entry_t* entry;
  void* arg;
  void* return_value;

  task* joiner;
  bool detached;
  std::chrono::microseconds vruntime;
  uint64_t priority_boost;

 private:
  // default constructor only for root task
  task();

  task(void* stack, void* stack_begin, size_t total_stack_size, bool alloc_tls);

  template <typename Thunk>
  void switch_to_internal(const Thunk* post_tls_switch_thunk);

  static end_handler _end_handler;
  static std::atomic_flag _lock;  // locks static variables
};
}  // namespace gthread

#include "sched/task_impl.h"
