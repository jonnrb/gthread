#include "sched/sched.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "platform/clock.h"

constexpr uint64_t k_num_threads = 1000;

using namespace gthread;

void* test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  auto& s = sched::get();
  for (auto start = thread_clock::now();
       thread_clock::now() - start < std::chrono::milliseconds{5};) {
    s.yield();
  }
  s.exit((void*)(i + 1));
  gthread_log_fatal("cannot be here!");
  return NULL;
}

void* sleepy_test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  auto& s = sched::get();
  s.sleep_for(std::chrono::milliseconds{50});
  s.exit((void*)(i + 1));
  gthread_log_fatal("cannot be here!");
  return NULL;
}

int main() {
  sched_handle threads[k_num_threads];

  auto& s = sched::get();

  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_threads << " threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      threads[i] = s.spawn(k_default_attr, test_thread, (void*)i);
    }

    std::cout << "joining ALL the threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      void* ret;
      s.join(&threads[i], &ret);
      if ((uint64_t)ret != i + 1) {
        gthread_log_fatal("incorrect return value");
      }
    }
  }

  std::cout << "timing this part" << std::endl;
  auto start = std::chrono::system_clock::now();
  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_threads << " sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      threads[i] = s.spawn(k_default_attr, sleepy_test_thread, (void*)i);
    }

    std::cout << "joining ALL the sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      void* ret;
      s.join(&threads[i], &ret);
      if ((uint64_t)ret != i + 1) {
        gthread_log_fatal("incorrect return value");
      }
    }
  }
  std::cout << "gthread took "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now() - start)
                   .count()
            << " µs" << std::endl;

  return 0;
}
