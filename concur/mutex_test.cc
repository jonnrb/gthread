/**
 * author: Khalid Akash, JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//concur/mutex_test.cc
 * info: tests mutex by locking and unlocking in a tight loop across threads
 */

#include "concur/mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "platform/clock.h"
#include "sched/sched.h"

static gthread::mutex mu{gthread::k_default_mutexattr};
static char last_task_with_mutex = '-';
static uint64_t mutex_tight_loops = 0;
static uint64_t mutextarget = 0;
static bool go = false;

constexpr uint64_t k_num_loops = 100 * 1000;
constexpr uint64_t k_num_inner_loops = 1000;

void* important_task(void* arg) {
  const char* msg = (const char*)arg;

  while (!go) gthread::sched::yield();

  for (int i = 0; i < k_num_loops; ++i) {
    if (branch_unexpected(mu.lock())) {
      gthread_log_fatal("lock failed");
    }

    for (int j = 0; j < k_num_inner_loops; ++j, ++i) {
      if (last_task_with_mutex != *msg) {
        std::cout << "mutex hot potato! " << last_task_with_mutex << " ("
                  << mutex_tight_loops << ") -> " << *msg << " * "
                  << gthread::task::current()->priority_boost << std::endl;
        mutex_tight_loops = 0;
      }
      ++mutex_tight_loops;
      last_task_with_mutex = *msg;

      mutextarget = mutextarget + 1;
    }

    if (branch_unexpected(mu.unlock())) {
      gthread_log_fatal("unlock failed");
    }
  }

  return NULL;
}

int main() {
  gthread::sched_handle tasks[26];
  char msgs[26] = {'A'};
  for (int i = 0; i < 26; ++i) {
    std::cout << "creating task " << i << std::endl;
    msgs[i] = msgs[0] + i;
    tasks[i] = gthread::sched::spawn(gthread::k_default_attr, important_task,
                                     (void*)(&msgs[i]));
    assert(tasks[i]);
  }

  go = true;

  for (int i = 0; i < 26; ++i) {
    gthread::sched::join(&tasks[i], nullptr);
  }
  std::cout << "done" << std::endl;

  if (mutextarget != 26 * k_num_loops) {
    gthread_log_fatal("missed target!");
  }

  return 0;
}
