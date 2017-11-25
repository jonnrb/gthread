/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.c
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "my_malloc/mymalloc.h"
#include "platform/clock.h"
#include "sched/sched.h"

void* tasksupersimple(void* _) {
  printf("TASK SUPER SIMPLE\n");

  printf("TASK SUPER SIMPLE: allocating\n");
  volatile int* p = (int*)mymalloc(1000 * sizeof(int), gthread_task_current());

  printf("TASK SUPER SIMPLE: writing\n");
  for (int i = 0; i < 1000; ++i) {
    p[i] = i;
  }

  printf("TASK SUPER SIMPLE: freeing\n");
  myfree((void*)p, gthread_task_current());

  return NULL;
}

void* task1(void* arg) {
  printf("TASK1 is %p\n", gthread_task_current());

  printf("allocating several blocks and then freeing them\n");
  void* p = mymalloc(10000, gthread_task_current());
  void* x = mymalloc(20000, gthread_task_current());
  void* z = mymalloc(10000, gthread_task_current());
  myfree(p, gthread_task_current());
  myfree(x, gthread_task_current());
  myfree(z, gthread_task_current());

  printInternalMemory(gthread_task_current());

  void* sp = shalloc(1000);
  myfree(sp, gthread_task_current());
  printShallocRegion();

  return NULL;
}

void* task2(void* arg) {
  printf("TASK2 is %p\n", gthread_task_current());

  printf("allocating 8MB; should work\n");
  void* p = mymalloc(8000000, gthread_task_current());
  assert(p != NULL);

  printf("allocating 8MB; should fail\n");
  void* x = mymalloc(8000000, gthread_task_current());
  assert(x == NULL);

  printf("freeing pointer %p\n", p);
  myfree(p, gthread_task_current());

  printf("freeing pointer %p (expecting failure)\n", x);
  myfree(x, gthread_task_current());

  printf("shalloc()ing 2K\n");
  void* s = shalloc(2000);
  assert(s != NULL);
  printShallocRegion();
  printf("freeing shalloc memory\n");
  myfree(s, gthread_task_current());

  return NULL;
}

void* task3(void* arg) {
  printf("TASK3 is %p\n", gthread_task_current());

  for (int i = 0; i < 3; ++i) {
    void* p = mymalloc(8000000, gthread_task_current());
    assert(p != NULL);
    printInternalMemory(gthread_task_current());
    myfree(p, gthread_task_current());
  }

  shalloc(3000);
  printShallocRegion();

  return NULL;
}

void* busy_task(void* arg) {
  unsigned* range = (unsigned*)arg;
  printf("starting busy_task (%p) on range [%u, %u)\n", gthread_task_current(),
         range[0], range[1]);

  unsigned size = range[1] - range[0];

  unsigned* arr =
      (unsigned*)mymalloc(size * sizeof(unsigned), gthread_task_current());
  printf("task %p using array at %p of size %zu\n", gthread_task_current(), arr,
         size * sizeof(unsigned));

  for (unsigned i = 0; i < size; ++i) {
    arr[i] = range[0] + i;
  }

  for (unsigned long long i = 0; i < 1000; ++i) {
    for (unsigned j = 0; j < size; ++j) {
      if (arr[j] != j + range[0]) {
        fprintf(stderr, "someone touched my (%p) memory!\n",
                gthread_task_current());
        abort();
      }
    }
  }

  myfree(arr, gthread_task_current());
  fprintf(stderr, "busy_task %p finished\n", gthread_task_current());

  return NULL;
}

int main() {
  int ret;

  gthread_sched_handle_t simple_task = NULL;
  ret = gthread_sched_spawn(&simple_task, NULL, tasksupersimple, NULL);
  assert(!ret);
  ret = gthread_sched_join(simple_task, NULL);
  assert(!ret);

  gthread_sched_handle_t parallel_tasks[8] = {NULL};

  ret = gthread_sched_spawn(&parallel_tasks[0], NULL, task1, NULL);
  assert(!ret);
  ret = gthread_sched_spawn(&parallel_tasks[1], NULL, task2, NULL);
  assert(!ret);
  ret = gthread_sched_spawn(&parallel_tasks[2], NULL, task3, NULL);
  assert(!ret);

  for (int i = 0; i < sizeof(parallel_tasks) / sizeof(gthread_sched_handle_t);
       ++i) {
    if (parallel_tasks[i] != NULL) {
      ret = gthread_sched_join(parallel_tasks[i], NULL);
      assert(!ret);
      parallel_tasks[i] = 0;
    }
  }

  unsigned ranges[9] = {0};
  ranges[0] = 0;
  for (int i = 0; i < sizeof(parallel_tasks) / sizeof(gthread_sched_handle_t);
       ++i) {
    ranges[i + 1] = 600 * 1000 * (i + 1);
    ret = gthread_sched_spawn(&parallel_tasks[i], NULL, busy_task, ranges + i);
    assert(!ret);
  }

  for (int i = 0; i < 3; ++i) {
    if (parallel_tasks[i] != NULL) {
      ret = gthread_sched_join(parallel_tasks[i], NULL);
      assert(!ret);
      parallel_tasks[i] = 0;
    }
  }

  printf("Test finished\n");
  return 0;
}
