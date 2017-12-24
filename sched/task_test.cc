#include "sched/task.h"

#include "gtest/gtest.h"
#include "sched/task_attr.h"

constexpr auto k_num_stress_tasks = 1000;
constexpr auto k_num_switches = 1000;

using namespace gthread;

void* thread(void* arg) {
  task* root = (task*)arg;
  for (int i = 0; i < k_num_switches; ++i) {
    root->switch_to();
  }
  return nullptr;
}

TEST(gthread_task, lots_of_switch_to) {
  task* tasks[k_num_stress_tasks] = {nullptr};
  task root;
  root.wrap_current();

  for (int i = 0; i < k_num_stress_tasks; ++i) {
    tasks[i] = task::create(k_default_attr);
    tasks[i]->entry = thread;
    tasks[i]->arg = (void*)&root;
    tasks[i]->start();
  }

  for (int i = 0; i < k_num_switches; ++i) {
    for (int j = 0; j < k_num_stress_tasks; ++j) {
      tasks[j]->switch_to();
    }
  }
}

TEST(gthread_task, lots_of_switch_to_no_tls) {
  task* tasks[k_num_stress_tasks] = {nullptr};
  task root{};
  root.wrap_current();

  for (int i = 0; i < k_num_stress_tasks; ++i) {
    tasks[i] = task::create(k_light_attr);
    tasks[i]->entry = thread;
    tasks[i]->arg = (void*)&root;
    tasks[i]->start();
  }

  for (int i = 0; i < k_num_switches; ++i) {
    for (int j = 0; j < k_num_stress_tasks; ++j) {
      tasks[j]->switch_to();
    }
  }
}
