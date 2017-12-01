#include "concur/mutex.h"

#include <cassert>
#include <cstdint>
#include <mutex>
#include <thread>

#include "gtest/gtest.h"
#include "gthread.h"
#include "platform/clock.h"

gthread::mutex mu;
char last_task_with_mutex = '-';
uint64_t mutex_tight_loops = 0;
uint64_t mutextarget = 0;
bool go = false;

constexpr uint64_t k_num_loops = 50 * 1000 * 1000;
constexpr uint64_t k_num_inner_loops = 1000 * 1000;

void important_task(char msg) {
  while (!go) gthread::self::yield();

  for (unsigned i = 0; i < k_num_loops; ++i) {
    if (branch_unexpected(mu.lock())) {
      gthread_log_fatal("lock failed");
    }

    for (unsigned j = 0; j < k_num_inner_loops; ++j, ++i) {
      if (last_task_with_mutex != msg) {
        std::cout << "mutex hot potato! " << last_task_with_mutex << " ("
                  << mutex_tight_loops << ") -> " << msg << " * "
                  << gthread::task::current()->priority_boost << std::endl;
        mutex_tight_loops = 0;
      }
      ++mutex_tight_loops;
      last_task_with_mutex = msg;

      ++mutextarget;
    }

    if (branch_unexpected(mu.unlock())) {
      gthread_log_fatal("unlock failed");
    }
  }
}

TEST(gthread_mutex, contention) {
  std::vector<gthread::g> tasks(10);
  int i = 0;
  for (auto& task : tasks) {
    std::cout << "creating task " << i << std::endl;
    task = gthread::g([](char msg) { important_task(msg); }, 'A' + i);
    assert((bool)task);
    ++i;
  }

  go = true;

  for (auto& task : tasks) {
    task.join();
  }
  std::cout << "done" << std::endl;

  if (mutextarget != 10 * k_num_loops) {
    gthread_log_fatal("missed target!");
  }
}
