/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/task_test.c
 * info: test task switching and spawning (WIP)
 */

#include "sched/sched.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "platform/clock.h"

#define k_num_threads 5000

static gthread_sched_handle_t threads[k_num_threads] = {NULL};

void* test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  for (uint64_t s = gthread_clock_process();
       gthread_clock_process() - s < (uint64_t)1000 * 1000 * 1000;) {
    gthread_sched_yield();
  }
  gthread_sched_exit((void*)(i + 1));
  assert(!"cannot be here!");
  return NULL;
}

int main() {
  printf("spawning %d threads\n", k_num_threads);
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    assert(!gthread_sched_spawn(&threads[i], NULL, test_thread, (void*)i));
  }

  printf("joining ALL the threads\n");
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    void* ret;
    assert(!gthread_sched_join(threads[i], &ret));
    assert((uint64_t)ret == i + 1);
  }

  return 0;
}
