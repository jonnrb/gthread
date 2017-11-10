#include "concur/mutex.h"

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "platform/clock.h"
#include "sched/sched.h"

gthread::mutex mu{gthread::k_default_mutexattr};
char last_task_with_mutex = '-';
uint64_t mutex_tight_loops = 0;
uint64_t mutextarget = 0;
bool go = false;

constexpr uint64_t k_num_loops = 100 * 1000;
constexpr uint64_t k_num_inner_loops = 1000;

void* important_task(void* arg) {
  const char* msg = (const char*)arg;

  while (!go) gthread::sched::get().yield();

  for (unsigned i = 0; i < k_num_loops; ++i) {
    if (branch_unexpected(mu.lock())) {
      gthread_log_fatal("lock failed");
    }

    for (unsigned j = 0; j < k_num_inner_loops; ++j, ++i) {
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
  auto& s = gthread::sched::get();
  char msgs[26] = {'A'};
  for (int i = 0; i < 26; ++i) {
    std::cout << "creating task " << i << std::endl;
    msgs[i] = msgs[0] + i;
    tasks[i] =
        s.spawn(gthread::k_default_attr, important_task, (void*)(&msgs[i]));
    assert(tasks[i]);
  }

  go = true;

  for (int i = 0; i < 26; ++i) {
    s.join(&tasks[i], nullptr);
  }
  std::cout << "done" << std::endl;

  if (mutextarget != 26 * k_num_loops) {
    gthread_log_fatal("missed target!");
  }

  return 0;
}
