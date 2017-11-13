#include "sched/task.h"

#include "gtest/gtest.h"
#include "sched/task_attr.h"

constexpr auto k_num_stress_tasks = 1000;
constexpr auto k_num_switches = 1000;

using namespace gthread;

void* thread(void* arg) {
  task* root = (task*)arg;
  for (int i = 0; i < k_num_switches; ++i) {
    EXPECT_EQ(root->switch_to(), 0);
  }
  return nullptr;
}

TEST(gthread_task, lots_of_switch_to) {
  task* tasks[k_num_stress_tasks] = {nullptr};
  task* root = task::current();
  for (int i = 0; i < k_num_stress_tasks; ++i) {
    tasks[i] = task::create(k_default_attr);
    tasks[i]->entry = thread;
    tasks[i]->arg = (void*)root;
    ASSERT_EQ(tasks[i]->start(), 0);
  }

  for (int i = 0; i < k_num_switches; ++i) {
    for (int j = 0; j < k_num_stress_tasks; ++j) {
      ASSERT_EQ(tasks[j]->switch_to(), 0);
    }
  }
}

TEST(gthread_task, lots_of_switch_to_no_tls) {
  task* tasks[k_num_stress_tasks] = {nullptr};
  task* root = task::current();
  for (int i = 0; i < k_num_stress_tasks; ++i) {
    tasks[i] = task::create(k_light_attr);
    tasks[i]->entry = thread;
    tasks[i]->arg = (void*)root;
    ASSERT_EQ(tasks[i]->start(), 0);
  }

  for (int i = 0; i < k_num_switches; ++i) {
    for (int j = 0; j < k_num_stress_tasks; ++j) {
      ASSERT_EQ(tasks[j]->switch_to(), 0);
    }
  }
}
