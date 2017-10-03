/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//sched/task.c
 * info: generic task switching functions for a scheduler
 */

#include "sched/task.h"

#include "arch/atomic.h"
#include "gthread.h"
#include "platform/memory.h"

// task representing the initial context of execution.
static gthread_task_t g_root_task = {0};

// were smp supported, we would actually need some per-processor distinction.
static gthread_task_t* g_current_task = NULL;

static gthread_task_end_handler_t* g_task_end_handler = NULL;

// task switching MUST not be reentrant.
static uint64_t g_lock = 0;

static int g_timer_enabled = 0;

static gthread_task_time_slice_trap_t* g_time_slice_trap = NULL;

int gthread_construct_task(gthread_task_t* task, gthread_attr_t* attrs) {
  int ret;
  task->run_state = GTHREAD_TASK_STOPPED;
  task->return_value = NULL;
  if ((ret = gthread_allocate_stack(attrs, &task->stack,
                                    &task->total_stack_size))) {
    return ret;
  }
  for (int i = 0; i < 16; ++i) task->slice_times[i] = 0;
  task->slice_i = 0;
  return 0;
}

static inline void init_root_task() {
  if (g_current_task == NULL) {
    g_current_task = &g_root_task;
    g_current_task->run_state = GTHREAD_TASK_RUNNING;
    // struct is statically zero-initialized as the lord penguin intended.
  }
}

static void record_time_slice(gthread_task_t* task, uint64_t elapsed) {
  task->slice_times[task->slice_i] = elapsed;
  task->slice_i = (task->slice_i + 1) % 16;
}

static void reset_timer_and_record_time(gthread_task_t* task) {
  if (g_timer_enabled) {
    uint64_t elapsed = gthread_timer_reset();
    record_time_slice(task, elapsed);
  }
}

typedef struct _entry_point {
  gthread_task_t* task;
  gthread_entry_t* entry;
  void* arg;
} entry_point_t;

static void task_entry(void* arg) {
  entry_point_t* entry_point = (entry_point_t*)arg;

  entry_point->task->run_state = GTHREAD_TASK_RUNNING;
  entry_point->task->return_value = entry_point->entry(entry_point->arg);
  entry_point->task->run_state = GTHREAD_TASK_STOPPED;

  if (g_task_end_handler) {
    g_task_end_handler(entry_point->task);
  }
}

int gthread_start_task(gthread_task_t* task, gthread_entry_t* entry,
                       void* arg) {
  if (task == g_current_task) {
    assert(0);  // DO NOT SUBMIT: test for now
    return 0;
  }

  init_root_task();

  reset_timer_and_record_time(g_current_task);

  if (!gthread_cas(&g_lock, 0, 1)) return -1;

  gthread_task_t* prev_task = g_current_task;
  g_current_task = task;

  prev_task->run_state = GTHREAD_TASK_SUSPENDED;
  g_current_task->run_state = GTHREAD_TASK_LOCKED;  // will be running in entry

  entry_point_t* entry_point = (entry_point_t*)g_current_task->stack - 1;
  entry_point->task = task;
  entry_point->entry = entry;
  entry_point->arg = arg;

  void* stack_head = (void*)((size_t)entry_point & ~((size_t)0xF));

  g_lock = 0;
  gthread_switch_to_and_spawn(&prev_task->ctx, &prev_task->ctx, stack_head,
                              task_entry, (void*)entry_point);
  prev_task->run_state = GTHREAD_TASK_RUNNING;
  return 0;
}

gthread_task_t* gthread_get_current_task() {
  init_root_task();
  return g_current_task;
}

static int switch_to_task(gthread_task_t* task, uint64_t* elapsed) {
  if (task == g_current_task) {
    assert(0);  // DO NOT SUBMIT: test for now
    return 0;
  }

  init_root_task();

  if (!gthread_cas(&g_lock, 0, 1)) return -1;

  if (elapsed == NULL) {
    reset_timer_and_record_time(g_current_task);
  } else {
    record_time_slice(g_current_task, *elapsed);
  }

  gthread_task_t* prev_task = g_current_task;
  g_current_task = task;

  prev_task->run_state = GTHREAD_TASK_SUSPENDED;

  g_lock = 0;
  gthread_switch_to(&prev_task->ctx, &g_current_task->ctx);

  assert(g_current_task == prev_task);
  prev_task->run_state = GTHREAD_TASK_RUNNING;
  return 0;
}

int gthread_switch_to_task(gthread_task_t* task) {
  return switch_to_task(task, NULL);
}

static void time_slice_trap(uint64_t elapsed) {
  gthread_task_t* next_task = NULL;

  if (g_time_slice_trap != NULL) {
    next_task = g_time_slice_trap(g_current_task);
  }

  if (next_task != NULL) {
    switch_to_task(next_task, &elapsed);
  } else {
    record_time_slice(g_current_task, elapsed);
  }
}

int gthread_task_set_time_slice_trap(gthread_task_time_slice_trap_t* trap,
                                     uint64_t usec) {
  g_timer_enabled = usec != 0;
  g_time_slice_trap = trap;
  gthread_timer_set_trap(time_slice_trap);
  return gthread_timer_set_interval(usec);
}

int gthread_set_task_end_handler(gthread_task_end_handler_t* task_end_handler) {
  g_task_end_handler = task_end_handler;
  return 0;
}
