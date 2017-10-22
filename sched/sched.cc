/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.cc
 * info: scheduler for uniprocessors
 */

#include "sched/sched.h"

#include <errno.h>
#include <inttypes.h>
#include <atomic>
#include <iostream>

#include "platform/clock.h"
#include "platform/memory.h"
#include "util/compiler.h"
#include "util/log.h"
#include "util/rb.h"

namespace gthread {
namespace {

template <bool enabled = false, uint64_t interval = 0>
struct stats {
  uint64_t start_time;
  uint64_t cur_start_time;
  uint64_t used_time;
  sched_handle handle;

  inline void maybe_start_interval() {
    if constexpr (enabled) {
      cur_start_time = gthread_clock_process();
    }
  }

  inline void maybe_end_interval() {
    if constexpr (enabled) {
      used_time += gthread_clock_process() - cur_start_time;
    }
  }

  void print() {
    std::cerr << "printing scheduler stats every "
              << (interval / (double)(1000 * 1000 * 1000)) << "s" << std::endl;

    uint64_t now = gthread_clock_process();
    uint64_t last = now;

    while (true) {
      if (now - last < (uint64_t)5 * 1000 * 1000 * 1000) {
        sched::yield();
        now = gthread_clock_process();
        continue;
      }
      last = now;

      std::cerr << "process time since sched start   " << (now - start_time)
                << "\nprocess time spent in scheduler  " << used_time
                << "\nratio spent in scheduler         "
                << ((double)used_time / now) << std::endl;
    }
  }

  static void* thread(stats<enabled, interval>* self) {
    self->print();
    return nullptr;
  }

  inline int maybe_init() {
    if constexpr (enabled) {
      start_time = gthread_clock_process();
      used_time = 0;

      handle = gthread_sched_spawn(nullptr, &decltype(this)::thread, this);
      if (!handle) {
        return -1;
      }
    }

    return 0;
  }
};

#ifdef GTHREAD_SCHED_COLLECT_STATS

static constexpr bool g_collect_stats = true;
#ifdef GTHREAD_SCHED_STATS_INTERVAL
static constexpr uint64_t g_stats_interval = 5 * 1000 * 1000 * 1000;
#else
static constexpr uint64_t g_stats_interval = GTHREAD_SCHED_STATS_INTERVAL;
#endif  // GTHREAD_SCHED_STATS_INTERVAL

#else

static constexpr bool g_collect_stats = false;
static constexpr uint64_t g_stats_interval = 0;

#endif  // GTHREAD_SCHED_COLLECT_STATS

static stats<g_collect_stats, g_stats_interval> g_stats;

}  // namespace

std::atomic<bool> sched::is_init = ATOMIC_VAR_INIT(false);
gthread_task_t* const sched::k_pointer_lock = (gthread_task_t*)-1;
std::atomic<gthread_task_t*> sched::interrupt_lock = ATOMIC_VAR_INIT(nullptr);
gthread_rb_tree_t sched::runqueue = nullptr;
uint64_t sched::min_vruntime = 0;

uint64_t sched::freelist_r = 0;
uint64_t sched::freelist_w = 0;
gthread_task_t* sched::freelist[k_freelist_size] = {NULL};

/**
 * pushes the |last_running_task| to the `runqueue` if it is in a runnable
 * state and pops the task with the least virtual runtime from `runqueue` to
 * return
 */
gthread_task_t* sched::next(gthread_task_t* last_running_task) {
  g_stats.maybe_start_interval();

  // if for some reason, a task that was about to switch gets interrupted,
  // switch to that task
  gthread_task_t* waiter = nullptr;
  if (branch_unexpected(
          !interrupt_lock.compare_exchange_strong(waiter, k_pointer_lock))) {
    if (waiter == k_pointer_lock) {
      gthread_log_fatal(
          "scheduler was interrupted by itself! this should be impossible. "
          "bug???");
    }
    g_stats.maybe_end_interval();
    return waiter;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (last_running_task->run_state == GTHREAD_TASK_RUNNING) {
    gthread_rb_push(&runqueue, &last_running_task->s.rq.rb_node);
  }

  gthread_rb_node_t* node = gthread_rb_pop_min(&runqueue);

  interrupt_lock = nullptr;

  // XXX: remove the assumption that the runqueue is never empty. this is true
  // if all tasks are sleeping and there is actually nothing to do. right now
  // it is impossible to be in this state without some sort of deadlock.
  if (branch_unexpected(node == NULL)) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  gthread_task_t* next_task = container_of(node, gthread_task_t, s.rq.rb_node);

  // the task that was just popped from the `runqueue` a priori is the task
  // with the minimum vruntime. since new tasks must start with a reasonable
  // vruntime, update `min_vruntime`.
  if (min_vruntime < next_task->s.rq.vruntime) {
    min_vruntime = next_task->s.rq.vruntime;
  }

  g_stats.maybe_end_interval();

  return next_task;
}

static void task_end_handler(gthread_task_t* task) {
  sched::exit(task->return_value);
}

int sched::init() {
  bool expected = false;
  if (!is_init.compare_exchange_strong(expected, true)) return -1;

  gthread_task_set_time_slice_trap(&sched::next, 50 * 1000);
  gthread_task_set_end_handler(task_end_handler);

  if (g_stats.maybe_init()) return -1;

  return 0;
}

gthread_task_t* sched::make_task(gthread_attr_t* attr) {
  gthread_task_t* task;

  g_stats.maybe_start_interval();

  // try to grab a task from the freelist
  if (freelist_w - freelist_r > 0) {
    uint64_t pos = freelist_r;
    task = freelist[pos % k_freelist_size];
    ++freelist_r;
    gthread_task_reset(task);
    task->s.rq.vruntime = min_vruntime;
    g_stats.maybe_end_interval();
    return task;
  }

  task = gthread_task_construct(attr);
  if (branch_unexpected(task == NULL)) {
    perror("task construction failed");
    g_stats.maybe_end_interval();
    return NULL;
  }

  g_stats.maybe_end_interval();

  return task;
}

void sched::return_task(gthread_task_t* task) {
  // don't deallocate task storage immediately if possible
  if (freelist_w - freelist_r < k_freelist_size) {
    freelist[freelist_w % k_freelist_size] = task;
    ++freelist_w;
  } else {
    gthread_task_destruct(task);
  }
}

int sched::yield() {
  if (!is_init) return -1;
  return gthread_timer_alarm_now();
}

// TODO: this would be nice (nanosleep sleeps the whole process)
// int64_t gthread_sched_nanosleep(uint64_t ns) {}

sched_handle sched::spawn(gthread_attr_t* attr, gthread_entry_t* entry,
                          void* arg) {
  sched_handle handle;

  if (branch_unexpected(entry == NULL)) {
    errno = EINVAL;
    return handle;
  }

  if (branch_unexpected(!is_init)) {
    if (init()) return handle;
  }

  handle.task = make_task(attr);
  if (branch_unexpected(!handle)) {
    return handle;
  }

  // this will start the task and immediately return control
  handle.task->entry = entry;
  handle.task->arg = arg;
  if (branch_unexpected(gthread_task_start(handle.task))) {
    return_task(handle.task);
    handle.task = nullptr;
    return handle;
  }

  uninterruptable_lock();
  gthread_rb_push(&runqueue, &handle.task->s.rq.rb_node);
  uninterruptable_unlock();

  return handle;
}

/**
 * waits for the task indicated by |handle| to finish
 *
 * cleans up |handle|'s task's memory after it has returned
 *
 * implementation details:
 *
 * this function is very basic except for a race condition. the task's `joiner`
 * property is used as a lock to prevent the race condition from leaving
 * something possibly permanently descheduled.
 *
 * if |handle|'s task has already exited, the function runs without blocking.
 * however, if |handle|'s task is still running, the `joiner` flag is locked to
 * the current task which will deschedule itself until |handle|'s task finishes.
 */
int sched::join(sched_handle* handle, void** return_value) {
  if (branch_unexpected(handle == nullptr || !*handle)) {
    return -1;
  }

  gthread_task_t* current = gthread_task_current();

  // flag to |thread| that you are the joiner
  for (gthread_task_t* current_joiner = nullptr;
       branch_unexpected(!handle->task->joiner.compare_exchange_strong(
           current_joiner, current));
       current_joiner = nullptr) {
    // if the joiner is not `nullptr` and is not locked, something else is
    // joining, which is undefined behavior
    if (branch_unexpected(current_joiner != (gthread_task_t*)k_pointer_lock &&
                          current_joiner != NULL)) {
      return -1;
    }

    yield();
  }

  // if we have locked `handle.task->joiner`, we can deschedule ourselves by
  // setting our `run_state` to "waiting" and yielding to the scheduler.
  // |handle|'s task will reschedule us when it has stopped, in which case the
  // loop will break and we know we can read off the return value and
  // (optionally) destroy the thread memory.
  while (handle->task->run_state != GTHREAD_TASK_STOPPED) {
    current->run_state = GTHREAD_TASK_WAITING;
    yield();
  }

  // take the return value of |thread| if |return_value| is given
  if (return_value != NULL) {
    *return_value = handle->task->return_value;
  }

  return_task(handle->task);
  handle->task = nullptr;

  return 0;
}

/**
 * immediately exits the current thread with return value |return_value|
 */
void sched::exit(void* return_value) {
  gthread_task_t* current = gthread_task_current();

  // indicative that the current task is the root task. abort reallly hard.
  if (branch_unexpected(current->stack == NULL)) {
    gthread_log_fatal("cannot exit from the root task!");
  }

  // lock the joiner with a flag
  gthread_task_t* joiner = nullptr;
  current->joiner.compare_exchange_strong(joiner, k_pointer_lock);

  current->return_value = return_value;       // save |return_value|
  current->run_state = GTHREAD_TASK_STOPPED;  // deschedule permanently

  if (joiner != NULL) {
    // consider the very weird case where join() locked `joiner` but hasn't
    // descheduled itself yet. it would be verrrryyy bad to put something in
    // the tree twice. if this task is running and `joiner` is in the waiting
    // state, it must be descheduled.
    while (branch_unexpected(joiner->run_state != GTHREAD_TASK_WAITING)) {
      yield();
    }

    uninterruptable_lock();
    gthread_rb_push(&runqueue, &joiner->s.rq.rb_node);
    uninterruptable_unlock();
  } else {
    // if there wasn't a joiner that suspended itself, we entered a critical
    // section and we should unlock
    current->joiner = NULL;
  }

  yield();  // deschedule

  // impossible to be here
  gthread_log_fatal("how did I get here?");
}

}  // namespace gthread
