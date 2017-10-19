/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/sched.c
 * info: scheduler for uniprocessors
 */

#include "sched/sched.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

#include "arch/atomic.h"
#include "platform/clock.h"
#include "platform/memory.h"
#include "util/compiler.h"
#include "util/rb.h"

#define k_pointer_lock ((uint64_t)-1)

static uint64_t g_is_sched_init = 0;

/**
 * tasks that can be switched to with the expectation that they will make
 * progress
 */
static uint64_t g_runqueue_lock = 0;
static gthread_rb_tree_t g_runqueue = NULL;
static gthread_rb_tree_t g_next_runqueue = NULL;

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
static uint64_t g_freelist_w = 0;  // TODO: writer
static gthread_task_t* g_freelist[k_freelist_size] = {NULL};

static uint64_t min_vruntime = 0;

#ifdef GTHREAD_SCHED_COLLECT_STATS
#ifndef GTHREAD_SCHED_STATS_INTERVAL
#define GTHREAD_SCHED_STATS_INTERVAL ((uint64_t)5 * 1000 * 1000 * 1000)
#endif  // GTHREAD_SCHED_STATS_INTERVAL

static struct {
  uint64_t start_time;
  uint64_t used_time;
} stats;
#endif  // GTHREAD_SCHED_COLLECT_STATS

/**
 * pushes the |last_running_task| to the `g_runqueue` if it is in a runnable
 * state and pops the task with the least virtual runtime to return
 */
static inline gthread_task_t* next_task_min_vruntime(
    gthread_task_t* last_running_task) {
#ifdef GTHREAD_SCHED_COLLECT_STATS
  uint64_t start = gthread_clock_process();
#endif  // GTHREAD_SCHED_COLLECT_STATS

  // if for some reason, a task that was about to switch gets interrupted,
  // switch to that task
  if (branch_unexpected(!gthread_cas(&g_runqueue_lock, 0, k_pointer_lock))) {
    if (g_runqueue_lock == k_pointer_lock) {
      printf(
          "contention on rb tree from uninterruptable code!\n"
          "scheduler is running for wayyyyy too long (bug?)\n");
      assert(0);
    }
    fprintf(stderr, "scheduler contention\n");
    return (gthread_task_t*)g_runqueue_lock;
  }

  // if the task was in a runnable state when the scheduler was invoked, push it
  // to the runqueue
  if (last_running_task->run_state == GTHREAD_TASK_RUNNING) {
    gthread_rb_push(&g_next_runqueue, &last_running_task->rb_node);
  }

  if (g_runqueue == NULL) {
    g_runqueue = g_next_runqueue;
    g_next_runqueue = NULL;
  }
  gthread_rb_node_t* node = gthread_rb_pop_min(&g_runqueue);

  g_runqueue_lock = 0;

  // XXX: remove the assumption that the runqueue is never empty
  if (branch_unexpected(node == NULL)) assert(0);

  gthread_task_t* next_task = container_of(node, gthread_task_t, rb_node);

  // the task that was just popped from the `g_runqueue` a priori is the task
  // with the minimum vruntime. since new tasks must start with a reasonable
  // vruntime, update `min_vruntime`.
  if (min_vruntime < next_task->vruntime) {
    min_vruntime = next_task->vruntime;
  }

#ifdef GTHREAD_SCHED_COLLECT_STATS
  stats.used_time += gthread_clock_process() - start;
#endif  // GTHREAD_SCHED_COLLECT_STATS

  return next_task;
}

static gthread_task_t* make_task(gthread_attr_t* attr) {
  gthread_task_t* task;

#ifdef GTHREAD_SCHED_COLLECT_STATS
  uint64_t start = gthread_clock_process();
#endif  // GTHREAD_SCHED_COLLECT_STATS

  // try to grab a task from the freelist
  if (g_freelist_w - g_freelist_r > 0) {
    uint64_t pos = g_freelist_r;
    task = g_freelist[pos % k_freelist_size];
    if (branch_unexpected(!gthread_cas(&g_freelist_r, pos, pos + 1))) {
      fprintf(stderr, "reader contention on single reader circular buffer!\n");
      assert(0);
    }
    gthread_task_reset(task);
    task->vruntime = min_vruntime;
    return task;
  }

  task = gthread_task_construct(attr);
  if (branch_unexpected(task == NULL)) {
    perror("task construction failed");
    return NULL;
  }

#ifdef GTHREAD_SCHED_COLLECT_STATS
  stats.used_time += gthread_clock_process() - start;
#endif  // GTHREAD_SCHED_COLLECT_STATS

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

// quite poor scheduler implementation: basic round robin.
static gthread_task_t* sched_timer(gthread_task_t* task) {
  return next_task_min_vruntime(task);
}

#ifdef GTHREAD_SCHED_COLLECT_STATS
static void* print_stats(void* _) {
  printf("printing scheduler stats every %.3fs\n",
         GTHREAD_SCHED_STATS_INTERVAL / (double)(1000 * 1000 * 1000));

  uint64_t now = gthread_clock_process();
  uint64_t last = now;

  while (true) {
    if (now - last < (uint64_t)5 * 1000 * 1000 * 1000) {
      gthread_sched_yield();
      now = gthread_clock_process();
      continue;
    }
    last = now;

    printf("process time since sched start   %.12" PRIu64 "\n",
           now - stats.start_time);
    printf("process time spent in scheduler  %.12" PRIu64 "\n",
           stats.used_time);
    printf("ratio spent in scheduler         %.9f\n",
           (double)stats.used_time / now);
  }
}
#endif  // GTHREAD_SCHED_COLLECT_STATS

static void task_end_handler(gthread_task_t* task) {
  gthread_sched_exit(task->return_value);
}

int gthread_sched_init() {
  if (!gthread_cas(&g_is_sched_init, 0, 1)) return -1;

  gthread_task_set_time_slice_trap(sched_timer, 10 * 1000);
  gthread_task_set_end_handler(task_end_handler);

#ifdef GTHREAD_SCHED_COLLECT_STATS
  stats.start_time = gthread_clock_process();
  stats.used_time = 0;

  static gthread_sched_handle_t stats_handle = NULL;
  if (gthread_sched_spawn(&stats_handle, NULL, print_stats, NULL)) {
    return -1;
  }
#endif  // GTHREAD_SCHED_COLLECT_STATS

  return 0;
}

int gthread_sched_yield() {
  if (!g_is_sched_init) return -1;
  return gthread_timer_alarm_now();
}

// int64_t gthread_sched_nanosleep(uint64_t ns) {
//   if (!g_is_sched_init) return -1;
//
//   gthread_task_t* current = gthread_task_current();
//
//   while (!gthread_cas(&g_runqueue_lock, 0, (uint64_t)current)) {
//     printf("contention on rb tree from uninterruptable code!\n");
//     assert(0);
//   }
//
//   g_runqueue_lock = 0;
// }

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg) {
  if (handle == NULL || entry == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (branch_unexpected(!g_is_sched_init)) {
    if (gthread_sched_init()) return -1;
  }

  gthread_task_t* task = make_task(attr);
  gthread_task_t* current = gthread_task_current();

  while (!gthread_cas(&g_runqueue_lock, 0, (uint64_t)current)) {
    printf("contention on rb tree from uninterruptable code!\n");
    assert(0);
  }
  gthread_rb_push(&g_next_runqueue, &current->rb_node);
  g_runqueue_lock = 0;

  if (gthread_task_start(task, entry, arg)) {
    return -1;
  }

  *handle = task;

  return 0;
}

// wait for thread termination
int gthread_sched_join(gthread_sched_handle_t thread, void** return_value) {
  // flag to |thread| that you are the joiner
  while (
      branch_unexpected(!gthread_cas((uint64_t*)&thread->joiner, (uint64_t)NULL,
                                     (uint64_t)gthread_task_current()))) {
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

void gthread_sched_exit(void* return_value) {
  gthread_task_t* current = gthread_task_current();
  gthread_task_t* joiner = NULL;

  // lock the joiner with a flag
  if (!gthread_cas((uint64_t*)&current->joiner, (uint64_t)NULL,
                   k_pointer_lock)) {
    joiner = current->joiner;
  }

  current->return_value = return_value;       // save |return_value|
  current->run_state = GTHREAD_TASK_STOPPED;  // deschedule permanently

  if (joiner != NULL) {
    if (branch_unexpected(
            !gthread_cas(&g_runqueue_lock, 0, (uint64_t)current))) {
      printf("contention on rb tree from uninterruptable code!\n");
      assert(0);
    }
    gthread_rb_push(&g_next_runqueue, &joiner->rb_node);
    g_runqueue_lock = 0;
  } else {
    // if there wasn't a joiner that suspended itself, we entered a critical
    // section and we should unlock
    current->joiner = NULL;
  }

  assert(!gthread_sched_yield());  // deschedule
}
