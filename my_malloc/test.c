/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.c
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

// run `make clean && make CFLAGS=-DLOG_DEBUG bin-test/my_malloc/test` to get
// debug logging for my_malloc

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "my_malloc.h"
#include "platform/clock.h"
#include "sched/sched.h"

void* tasksupersimple(void* _) {
  printf("TASK SUPER SIMPLE\n");

  printf("TASK SUPER SIMPLE: allocating\n");
  volatile int* p = (int*)malloc(1000 * sizeof(int));

  printf("TASK SUPER SIMPLE: writing\n");
  for (int i = 0; i < 1000; ++i) {
    p[i] = i;
  }

  printf("TASK SUPER SIMPLE: freeing\n");
  free((void*)p);

  return NULL;
}

void* task1(void* arg) {
  printf("TASK1 is %p\n", gthread_task_current());

  printf("allocating several blocks and then freeing them\n");
  void* p = malloc(10000);
  void* x = malloc(20000);
  void* z = malloc(10000);
  free(p);
  free(x);
  free(z);

  printInternalMemory(gthread_task_current());

  void* sp = shalloc(1000);
  free(sp);
  printShallocRegion();

  return NULL;
}

void* task2(void* arg) {
  printf("TASK2 is %p\n", gthread_task_current());

  printf("allocating 8MB; should work\n");
  void* p = malloc(8000000);
  assert(p != NULL);

  printf("allocating 8MB; should fail\n");
  void* x = malloc(8000000);
  assert(x == NULL);

  printf("freeing pointer %p\n", p);
  free(p);

  printf("freeing pointer %p (expecting failure)\n", x);
  free(x);

  printf("shalloc()ing 2K\n");
  void* s = shalloc(2000);
  assert(s != NULL);
  printShallocRegion();
  printf("freeing shalloc memory\n");
  free(s);

  return NULL;
}

void* task3(void* arg) {
  printf("TASK3 is %p\n", gthread_task_current());

  for (int i = 0; i < 3; ++i) {
    void* p = malloc(8000000);
    assert(p != NULL);
    printInternalMemory(gthread_task_current());
    free(p);
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

  unsigned* arr = (unsigned*)malloc(size * sizeof(unsigned));
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

  free(arr);
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
