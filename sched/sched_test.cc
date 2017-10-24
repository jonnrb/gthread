/**
 * author: JonNRb <jonbetti@gmail.com>, Matthew Handzy <matthewhandzy@gmail.com>
 * license: MIT
 * file: @gthread//sched/task_test.cc
 * info: test task switching and spawning (WIP)
 */

#include "sched/sched.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "platform/clock.h"

#define k_num_threads 5000

using namespace gthread;

static sched_handle threads[k_num_threads];

void* test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  for (uint64_t s = gthread_clock_process();
       gthread_clock_process() - s < (uint64_t)1000 * 1000;) {
    sched::yield();
  }
  sched::exit((void*)(i + 1));
  assert(!"cannot be here!");
  return NULL;
}

int main() {
  printf("spawning %d threads\n", k_num_threads);
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    threads[i] = sched::spawn(NULL, test_thread, (void*)i);
  }

  printf("joining ALL the threads\n");
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    void* ret;
    sched::join(&threads[i], &ret);
    if ((uint64_t)ret != i + 1) {
      gthread_log_fatal("incorrect return value");
    }
  }

  return 0;
}
