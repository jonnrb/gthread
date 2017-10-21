/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.cc
 * info: scheduler for uniprocessors
 */

#include "sched/sched.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <atomic>

#include "platform/clock.h"
#include "platform/memory.h"
#include "util/compiler.h"
#include "util/log.h"
#include "util/rb.h"

#define k_pointer_lock ((gthread_task_t*)-1)

std::atomic<bool> g_is_sched_init = ATOMIC_VAR_INIT(false);

std::atomic<gthread_task_t*> g_interrupt_lock = ATOMIC_VAR_INIT(nullptr);

/**
 * tasks that can be switched to with the expectation that they will make
 * progress
 */
gthread_rb_tree_t gthread_sched_runqueue = NULL;

/**
 * `g_next_sleepqueue_wake` has the next time at which the `g_sleepqueue`
 * should be accessed
 */
// static uint64_t g_sleepqueue_lock = 0;
// static uint64_t g_next_sleepqueue_wake = 0;
// static gthread_rb_tree_t g_sleepqueue = NULL;

// ring buffer for free tasks
#define k_freelist_size 64
static uint64_t g_freelist_r = 0;  // reader is `make_task()`
static uint64_t g_freelist_w = 0;  // writer is `return_task()`
static gthread_task_t* g_freelist[k_freelist_size] = {NULL};

static uint64_t min_vruntime = 0;

#ifdef GTHREAD_SCHED_COLLECT_STATS

#ifndef GTHREAD_SCHED_STATS_INTERVAL
#define GTHREAD_SCHED_STATS_INTERVAL ((uint64_t)5 * 1000 * 1000 * 1000)
#endif  // GTHREAD_SCHED_STATS_INTERVAL

static struct {
  uint64_t start_time;
  uint64_t cur_start_time;
  uint64_t used_time;
} g_stats;

static inline void maybe_start_stats_interval() {
  g_stats.cur_start_time = gthread_clock_process();
}

static inline void maybe_end_stats_interval() {
  g_stats.used_time += gthread_clock_process() - g_stats.cur_start_time;
}

static void* print_stats(void* _) {
  fprintf(stderr, "printing scheduler stats every %.3fs\n",
          GTHREAD_SCHED_STATS_INTERVAL / (double)(1000 * 1000 * 1000));
  fflush(stderr);

  uint64_t now = gthread_clock_process();
  uint64_t last = now;

  while (true) {
    if (now - last < (uint64_t)5 * 1000 * 1000 * 1000) {
      gthread_sched_yield();
      now = gthread_clock_process();
      continue;
    }
    last = now;

    fprintf(stderr, "process time since sched start   %.12" PRIu64 "\n",
            now - g_stats.start_time);
    fprintf(stderr, "process time spent in scheduler  %.12" PRIu64 "\n",
            g_stats.used_time);
    fprintf(stderr, "ratio spent in scheduler         %.9f\n",
            (double)g_stats.used_time / now);
    fflush(stderr);
  }
}

static inline int maybe_init_stats() {
  g_stats.start_time = gthread_clock_process();
  g_stats.used_time = 0;

  static gthread_sched_handle_t stats_handle = NULL;
  if (gthread_sched_spawn(&stats_handle, NULL, print_stats, NULL)) {
    return -1;
  }

  return 0;
}

#else

static inline void maybe_start_stats_interval() {}
static inline void maybe_end_stats_interval() {}
static inline int maybe_init_stats() { return 0; }

#endif  // GTHREAD_SCHED_COLLECT_STATS

/**
 * pushes the |last_running_task| to the `gthread_sched_runqueue` if it is in a
 * runnable state and pops the task with the least virtual runtime from
 * `gthread_sched_runqueue` to return
 */
static inline gthread_task_t* sched(gthread_task_t* last_running_task) {
  maybe_start_stats_interval();

  // if for some reason, a task that was about to switch gets interrupted,
  // switch to that task
  gthread_task_t* waiter = nullptr;
  if (branch_unexpected(
          !g_interrupt_lock.compare_exchange_strong(waiter, k_pointer_lock))) {
    if (waiter == k_pointer_lock) {
      gthread_log_fatal(
          "scheduler was interrupted by itself! this should be impossible. "
          "bug???");
    }
    maybe_end_stats_interval();
    return waiter;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (last_running_task->run_state == GTHREAD_TASK_RUNNING) {
    gthread_rb_push(&gthread_sched_runqueue, &last_running_task->s.rq.rb_node);
  }

  gthread_rb_node_t* node = gthread_rb_pop_min(&gthread_sched_runqueue);

  g_interrupt_lock = nullptr;

  // XXX: remove the assumption that the runqueue is never empty. this is true
  // if all tasks are sleeping and there is actually nothing to do. right now
  // it is impossible to be in this state without some sort of deadlock.
  if (branch_unexpected(node == NULL)) {
    gthread_log_fatal("nothing to do. deadlock?");
  }

  gthread_task_t* next_task = container_of(node, gthread_task_t, s.rq.rb_node);

  // the task that was just popped from the `gthread_sched_runqueue` a priori is
  // the task with the minimum vruntime. since new tasks must start with a
  // reasonable vruntime, update `min_vruntime`.
  if (min_vruntime < next_task->s.rq.vruntime) {
    min_vruntime = next_task->s.rq.vruntime;
  }

  maybe_end_stats_interval();

  return next_task;
}

static void task_end_handler(gthread_task_t* task) {
  gthread_sched_exit(task->return_value);
}

static inline int init_sched_module() {
  bool expected = false;
  if (!g_is_sched_init.compare_exchange_strong(expected, true)) return -1;

  gthread_task_set_time_slice_trap(sched, 50 * 1000);
  gthread_task_set_end_handler(task_end_handler);

  if (maybe_init_stats()) return -1;

  return 0;
}

static gthread_task_t* make_task(gthread_attr_t* attr) {
  gthread_task_t* task;

  maybe_start_stats_interval();

  // try to grab a task from the freelist
  if (g_freelist_w - g_freelist_r > 0) {
    uint64_t pos = g_freelist_r;
    task = g_freelist[pos % k_freelist_size];
    ++g_freelist_r;
    gthread_task_reset(task);
    task->s.rq.vruntime = min_vruntime;
    maybe_end_stats_interval();
    return task;
  }

  task = gthread_task_construct(attr);
  if (branch_unexpected(task == NULL)) {
    perror("task construction failed");
    maybe_end_stats_interval();
    return NULL;
  }

  maybe_end_stats_interval();

  return task;
}

static void return_task(gthread_task_t* task) {
  // don't deallocate task storage immediately if possible
  if (g_freelist_w - g_freelist_r < k_freelist_size) {
    g_freelist[g_freelist_w % k_freelist_size] = task;
    ++g_freelist_w;
  } else {
    gthread_task_destruct(task);
  }
}

int gthread_sched_yield() {
  if (!g_is_sched_init) return -1;
  return gthread_timer_alarm_now();
}

// TODO: this would be nice (nanosleep sleeps the whole process)
// int64_t gthread_sched_nanosleep(uint64_t ns) {}

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg) {
  if (branch_unexpected(handle == NULL || entry == NULL)) {
    errno = EINVAL;
    return -1;
  }

  if (branch_unexpected(!g_is_sched_init)) {
    if (init_sched_module()) return -1;
  }

  gthread_task_t* task = make_task(attr);
  task->entry = entry;
  task->arg = arg;

  if (branch_unexpected(gthread_task_start(task))) {
    return -1;
  }

  gthread_sched_uninterruptable_lock();
  gthread_rb_push(&gthread_sched_runqueue, &task->s.rq.rb_node);
  gthread_sched_uninterruptable_unlock();

  *handle = task;
  return 0;
}

/**
 * waits for |thread| to finish
 *
 * cleans up |thread|'s memory after it has returned
 *
 * implementation details:
 *
 * this function is very basic except for a race condition. the |thread|'s
 * `joiner` property is used as a lock to prevent the race condition from
 * leaving something possibly permanently descheduled.
 *
 * if |thread| has already exited, the function runs without blocking. however,
 * if |thread| is still running, the `joiner` flag is locked to the current
 * task and this task will deschedule itself.
 */
int gthread_sched_join(gthread_sched_handle_t thread, void** return_value) {
  // flag to |thread| that you are the joiner
  for (gthread_task_t* expected = nullptr;
       branch_unexpected(!thread->joiner.compare_exchange_strong(
           expected, gthread_task_current()));
       expected = nullptr) {
    // if the joiner is not NULL and is not locked, something else is joining,
    // which is undefined behavior
    gthread_task_t* current_joiner = thread->joiner;
    if (branch_unexpected(current_joiner != (gthread_task_t*)k_pointer_lock &&
                          current_joiner != NULL)) {
      return -1;
    } else {
      gthread_sched_yield();
    }
  }

  // if we have locked `thread->joiner`, we can deschedule ourselves by setting
  // our `run_state` to "waiting" and yielding to the scheduler. |thread| will
  // reschedule us when it has stopped, in which case the loop will break and we
  // know we can read off the return value and (optionally) destroy the thread
  // memory.
  while (thread->run_state != GTHREAD_TASK_STOPPED) {
    gthread_task_current()->run_state = GTHREAD_TASK_WAITING;
    gthread_sched_yield();
  }

  // take the return value of |thread| if |return_value| is given
  if (return_value != NULL) {
    *return_value = thread->return_value;
  }

  return_task(thread);

  return 0;
}

/**
 * immediately exits the current thread with return value |return_value|
 */
void gthread_sched_exit(void* return_value) {
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
      gthread_sched_yield();
    }

    gthread_sched_uninterruptable_lock();
    gthread_rb_push(&gthread_sched_runqueue, &joiner->s.rq.rb_node);
    gthread_sched_uninterruptable_unlock();
  } else {
    // if there wasn't a joiner that suspended itself, we entered a critical
    // section and we should unlock
    current->joiner = NULL;
  }

  assert(!gthread_sched_yield());  // deschedule
}
