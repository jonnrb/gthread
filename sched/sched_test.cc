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

constexpr uint64_t k_num_threads = 5000;

using namespace gthread;

void* test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  for (uint64_t s = gthread_clock_process();
       gthread_clock_process() - s < (uint64_t)1000 * 1000;) {
    sched::yield();
  }
  sched::exit((void*)(i + 1));
  gthread_log_fatal("cannot be here!");
  return NULL;
}

int main() {
  sched_handle threads[k_num_threads];

  std::cout << "spawning " << k_num_threads << " threads" << std::endl;
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    threads[i] = sched::spawn(k_default_attr, test_thread, (void*)i);
  }

  std::cout << "joining ALL the threads" << std::endl;
  for (uint64_t i = 0; i < k_num_threads; ++i) {
    void* ret;
    sched::join(&threads[i], &ret);
    if ((uint64_t)ret != i + 1) {
      gthread_log_fatal("incorrect return value");
    }
  }

  return 0;
}
