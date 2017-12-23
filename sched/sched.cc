#include "sched/sched.h"

#include <atomic>
#include <cassert>
#include <cerrno>
#include <cinttypes>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

#include "platform/clock.h"
#include "platform/memory.h"
#include "sched/preempt.h"
#include "util/compiler.h"
#include "util/log.h"

namespace gthread {
namespace {
void task_end_handler(task* task) { sched::exit(task->return_value); }

/**
 * represents the global scheduler context
 */
struct sched_context {
  std::shared_ptr<internal::task_freelist> freelist;
  sched_node root_node;  // TODO: add thread hosted nodes l8r

  sched_context()
      : freelist(std::make_shared<internal::task_freelist>(64)),
        root_node(freelist) {
    root_node.start_async();
    enable_timer_preemption();
    task::set_end_handler(task_end_handler);
  }

  /**
   * initialized on first use
   */
  static sched_context& get() {
    static sched_context instance;
    return instance;
  }
};
}  // namespace

namespace sched {
void yield() {
  auto* node = sched_node::current();
  if (node == nullptr) {
    std::cout << "awefawefgaehruiaew" << std::endl;
    return;
  }
  node->yield();
}

handle spawn(const attr& attr, task::entry_t* entry, void* arg) {
  handle handle;

  if (branch_unexpected(entry == nullptr)) {
    throw std::domain_error("must supply entry function");
  }

  auto& ctx = sched_context::get();
  handle.t = ctx.freelist->make_task(attr);
  handle.t->entry = entry;
  handle.t->arg = arg;

  // this will start the task and immediately return control
  handle.t->start();

  // NOTE: where do we schedule when there are options?
  auto* node = sched_node::current();
  assert(node != nullptr);
  {
    std::lock_guard<sched_node> l(*node);
    node->schedule(handle.t);
  }

  return handle;
}

void join(handle* handle, void** return_value) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    throw std::domain_error(
        "|handle| must be specified and must be a valid thread");
  }

  auto* current = task::current();
  auto* node = sched_node::current();  // XXX: we'll need a double checked lock
                                       // eventually

  {
    std::unique_lock<sched_node> l(*node);

    // if the joiner is not `nullptr`, something else is joining, which is
    // undefined behavior
    if (branch_unexpected(handle->t->joiner != nullptr)) {
      throw std::logic_error("a thread can only be joined from one place");
    }
    if (handle->t->run_state != task::STOPPED) {
      handle->t->joiner = current;
      current->run_state = task::WAITING;
      l.unlock();
      node->yield();
    }
  }

  assert(handle->t->run_state == task::STOPPED);

  // take the return value of |thread| if |return_value| is given
  if (return_value != nullptr) {
    *return_value = handle->t->return_value;
  }

  sched_context::get().freelist->return_task(handle->t);
  handle->t = nullptr;
}

void detach(handle* handle) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    throw std::domain_error(
        "|handle| must be specified and must be a valid thread");
  }

  auto* node = sched_node::current();  // XXX: we'll need a double checked lock
                                       // eventually

  {
    std::lock_guard<sched_node> l(*node);
    if (handle->t->run_state != task::STOPPED) {
      handle->t->detached = true;
      handle->t = nullptr;
      return;
    }
  }

  assert(handle->t->run_state == task::STOPPED);

  sched_context::get().freelist->return_task(handle->t);
  handle->t = nullptr;
}

void exit(void* return_value) {
  auto* current = task::current();
  auto* node = sched_node::current();  // XXX: we'll need a double checked lock
                                       // eventually

  // indicative that the current task is the root task. abort reallly hard.
  if (branch_unexpected(current->entry == nullptr)) {
    gthread_log_fatal("cannot exit from the root task!");
  }

  {
    std::lock_guard<sched_node> l(*node);
    current->return_value = return_value;  // save |return_value|
    current->run_state = task::STOPPED;    // deschedule permanently

    if (current->joiner != nullptr) {
      node->schedule(current->joiner);
    }
  }

  node->yield();  // deschedule

  // impossible to be here
  gthread_log_fatal("how did I get here?");
}
}  // namespace sched
}  // namespace gthread
