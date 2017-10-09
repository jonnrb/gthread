/**
 * author: JonNRb <jonbetti@gmail.com>
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
#include "util/compiler.h"
#include "util/rb.h"

static uint64_t g_is_sched_init = 0;

/**
 * tasks that can be switched to with the expectation that they will make
 * progress
 */
static uint64_t runqueue_lock = 0;
#define k_runqueue_locked_uninter 1
static gthread_rb_tree_t runqueue = NULL;

// ring buffer for free tasks
#define k_freelist_size 1024
static uint64_t freelist_r = 0;  // reader is `make_task()`
static uint64_t freelist_w = 0;  // TODO: writer
static gthread_task_t* freelist[k_freelist_size] = {NULL};

static uint64_t min_vruntime = 0;

static struct {
  uint64_t start_time;
  uint64_t used_time;
} stats;

/**
 * pushes the |last_running_task| to the `runqueue` and pops the task with the
 * least virtual runtime to return
 */
static inline gthread_task_t* next_task_min_vruntime(
    gthread_task_t* last_running_task) {
  uint64_t start = gthread_clock_process();

  if (branch_unexpected(!gthread_cas(&runqueue_lock, 0, 1))) {
    if (runqueue_lock == k_runqueue_locked_uninter) {
      printf(
          "contention on rb tree from uninterruptable code!\n"
          "scheduler is running for wayyyyy too long (bug?)\n");
      assert(0);
    }
    return (gthread_task_t*)runqueue_lock;
  }

  gthread_rb_push(&runqueue, &last_running_task->rb_node);
  gthread_rb_node_t* node = gthread_rb_pop_min(&runqueue);

  runqueue_lock = 0;

  if (branch_unexpected(node == NULL)) assert(0);
  gthread_task_t* next_task = container_of(node, gthread_task_t, rb_node);
  if (min_vruntime < next_task->vruntime) {
    min_vruntime = next_task->vruntime;
  }

  stats.used_time += gthread_clock_process() - start;

  return next_task;
}

static gthread_task_t* make_task(gthread_attr_t* attr) {
  gthread_task_t* task;

  uint64_t start = gthread_clock_process();

  // try to grab a task from the freelist
  if (freelist_w - freelist_r > 0) {
    task = freelist[freelist_r % k_freelist_size];
    if (branch_unexpected(
            !gthread_cas(&freelist_r, freelist_r, freelist_r + 1))) {
      assert(0);
    }
    gthread_task_reset(task);
    gthread_rb_construct(&task->rb_node);
    task->vruntime = min_vruntime;
    return task;
  }

  task = gthread_task_construct(attr);
  if (branch_unexpected(task == NULL)) {
    perror("task construction failed");
    return NULL;
  }

  stats.used_time += gthread_clock_process() - start;

  return task;
}

// quite poor scheduler implementation: basic round robin.
static gthread_task_t* sched_timer(gthread_task_t* task) {
  return next_task_min_vruntime(task);
}

int gthread_sched_init() {
  if (!gthread_cas(&g_is_sched_init, 0, 1)) return -1;

  gthread_task_set_time_slice_trap(sched_timer, 10 * 1000);

  stats.start_time = gthread_clock_process();
  stats.used_time = 0;

  return 0;
}

void gthread_sched_print_stats() {
  uint64_t now = gthread_clock_process();
  printf("time since sched start   %.12" PRIu64 "\n", now - stats.start_time);
  printf("time spent in scheduler  %.12" PRIu64 "\n", stats.used_time);
  printf("ratio spent in scheduler %.9f\n", (double)stats.used_time / now);
}

int gthread_sched_yield() {
  if (!g_is_sched_init) return -1;
  return gthread_timer_alarm_now();
}

int gthread_sched_spawn(gthread_sched_handle_t* handle, gthread_attr_t* attr,
                        gthread_entry_t* entry, void* arg) {
  if (handle == NULL || entry == NULL) {
    errno = EINVAL;
    return -1;
  }

  gthread_task_t* task = make_task(attr);
  gthread_task_t* current = gthread_task_current();

  while (!gthread_cas(&runqueue_lock, 0, (uint64_t)current)) {
    printf("contention on rb tree from uninterruptable code!\n");
    assert(0);
  }
  gthread_rb_push(&runqueue, &current->rb_node);
  runqueue_lock = 0;

  if (gthread_task_start(task, entry, arg)) {
    return -1;
  }

  *handle = task;

  return 0;
}
