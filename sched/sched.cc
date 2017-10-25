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
#include <set>

#include "platform/clock.h"
#include "platform/memory.h"
#include "util/compiler.h"
#include "util/log.h"

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

std::atomic<bool> sched::is_init{false};
task* const sched::k_pointer_lock = (task*)-1;
std::atomic<task*> sched::interrupt_lock{nullptr};

std::multiset<task*, sched::time_ordered_compare> sched::runqueue;
uint64_t sched::min_vruntime = 0;

uint64_t sched::freelist_r = 0;
uint64_t sched::freelist_w = 0;
task* sched::freelist[k_freelist_size] = {NULL};

/**
 * pushes the |last_running_task| to the `runqueue` if it is in a runnable
 * state and pops the task with the least virtual runtime from `runqueue` to
 * return
 */
task* sched::next(task* last_running_task) {
  g_stats.maybe_start_interval();

  // if for some reason, a task that was about to switch gets interrupted,
  // switch to that task
  task* waiter = nullptr;
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
  if (last_running_task->run_state == task::RUNNING) {
    runqueue_push(last_running_task);
  }

  // XXX: remove the assumption that the runqueue is never empty. this is true
  // if all tasks are sleeping and there is actually nothing to do. right now
  // it is impossible to be in this state without some sort of deadlock.
  if (branch_unexpected(runqueue.empty())) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  auto begin = runqueue.begin();
  task* next_task = *begin;
  runqueue.erase(begin);

  interrupt_lock = nullptr;

  // the task that was just popped from the `runqueue` a priori is the task
  // with the minimum vruntime. since new tasks must start with a reasonable
  // vruntime, update `min_vruntime`.
  if (min_vruntime < next_task->vruntime) {
    min_vruntime = next_task->vruntime;
  }

  g_stats.maybe_end_interval();

  return next_task;
}

static void task_end_handler(task* task) { sched::exit(task->return_value); }

int sched::init() {
  bool expected = false;
  if (!is_init.compare_exchange_strong(expected, true)) return -1;

  task::set_time_slice_trap(&sched::next, 50 * 1000);
  task::set_end_handler(task_end_handler);

  if (g_stats.maybe_init()) return -1;

  return 0;
}

task* sched::make_task(const attr& a) {
  task* t;

  g_stats.maybe_start_interval();

  // try to grab a task from the freelist
  if (freelist_w - freelist_r > 0) {
    uint64_t pos = freelist_r;
    t = freelist[pos % k_freelist_size];
    ++freelist_r;
    t->reset();
    t->vruntime = min_vruntime;
    g_stats.maybe_end_interval();
    return t;
  }

  t = new task(a);
  if (branch_unexpected(t == nullptr)) {
    perror("task construction failed");
    g_stats.maybe_end_interval();
    return nullptr;
  }

  g_stats.maybe_end_interval();

  return t;
}

void sched::return_task(task* t) {
  // don't deallocate task storage immediately if possible
  if (freelist_w - freelist_r < k_freelist_size) {
    freelist[freelist_w % k_freelist_size] = t;
    ++freelist_w;
  } else {
    delete t;
  }
}

int sched::yield() {
  if (!is_init) return -1;
  return gthread_timer_alarm_now();
}

// TODO: this would be nice (nanosleep sleeps the whole process)
// int64_t gthread_sched_nanosleep(uint64_t ns) {}

sched_handle sched::spawn(const attr& attr, task::entry_t* entry, void* arg) {
  sched_handle handle;

  if (branch_unexpected(entry == NULL)) {
    errno = EINVAL;
    return handle;
  }

  if (branch_unexpected(!is_init)) {
    if (init()) return handle;
  }

  handle.t = make_task(attr);
  if (branch_unexpected(!handle)) {
    return handle;
  }
  handle.t->entry = entry;
  handle.t->arg = arg;

  uninterruptable_lock();

  // this will start the task and immediately return control
  if (branch_unexpected(handle.t->start())) {
    return_task(handle.t);
    handle.t = nullptr;
    return handle;
  }

  runqueue_push(handle.t);

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

  task* current = task::current();

  // flag to |thread| that you are the joiner
  for (task* current_joiner = nullptr; branch_unexpected(
           !handle->t->joiner.compare_exchange_strong(current_joiner, current));
       current_joiner = nullptr) {
    // if the joiner is not `nullptr` and is not locked, something else is
    // joining, which is undefined behavior
    if (branch_unexpected(current_joiner != (task*)k_pointer_lock &&
                          current_joiner != nullptr)) {
      return -1;
    }

    yield();
  }

  // if we have locked `handle.task->joiner`, we can deschedule ourselves by
  // setting our `run_state` to "waiting" and yielding to the scheduler.
  // |handle|'s task will reschedule us when it has stopped, in which case the
  // loop will break and we know we can read off the return value and
  // (optionally) destroy the thread memory.
  while (handle->t->run_state != task::STOPPED) {
    current->run_state = task::WAITING;
    yield();
  }

  // take the return value of |thread| if |return_value| is given
  if (return_value != NULL) {
    *return_value = handle->t->return_value;
  }

  return_task(handle->t);
  handle->t = nullptr;

  return 0;
}

/**
 * immediately exits the current thread with return value |return_value|
 */
void sched::exit(void* return_value) {
  task* current = task::current();

  // indicative that the current task is the root task. abort reallly hard.
  if (branch_unexpected(current->stack == NULL)) {
    gthread_log_fatal("cannot exit from the root task!");
  }

  // lock the joiner with a flag
  task* joiner = nullptr;
  current->joiner.compare_exchange_strong(joiner, k_pointer_lock);

  current->return_value = return_value;  // save |return_value|
  current->run_state = task::STOPPED;    // deschedule permanently

  if (joiner != NULL) {
    // consider the very weird case where join() locked `joiner` but hasn't
    // descheduled itself yet. it would be verrrryyy bad to put something in
    // the tree twice. if this task is running and `joiner` is in the waiting
    // state, it must be descheduled.
    while (branch_unexpected(joiner->run_state != task::WAITING)) {
      yield();
    }

    uninterruptable_lock();
    runqueue_push(joiner);
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
