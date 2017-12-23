#include "sched/sched.h"

#include <atomic>
#include <cstdint>

#include "gtest/gtest.h"
#include "platform/clock.h"

constexpr uint64_t k_num_tasks = 1000;
constexpr uint64_t k_num_stress_tasks = 10000;

using namespace gthread;

void* returner(void* arg) {
  std::cout << "hi" << std::endl;
  auto i = reinterpret_cast<intptr_t>(arg);
  return reinterpret_cast<void*>(i + 1);
}

TEST(gthread_sched, basic_spawn_and_join) {
  auto h = sched::spawn(k_default_attr, returner, reinterpret_cast<void*>(5));
  void* ret;
  sched::join(&h, &ret);
  EXPECT_EQ(reinterpret_cast<intptr_t>(ret), 6);
}

void* test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  for (auto start = thread_clock::now();
       thread_clock::now() - start < std::chrono::milliseconds{1};) {
    sched::yield();
  }
  sched::exit((void*)(i + 1));
  gthread_log_fatal("cannot be here!");
  return NULL;
}

TEST(gthread_sched, lots_of_yields) {
  sched::handle threads[k_num_tasks];

  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_tasks << " threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      threads[i] = sched::spawn(k_default_attr, test_thread, (void*)i);
    }

    std::cout << "joining ALL the threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      void* ret;
      sched::join(&threads[i], &ret);
      EXPECT_EQ((uint64_t)ret, i + 1);
    }
  }
}

void* sleepy_test_thread(void* arg) {
  uint64_t i = (uint64_t)arg;
  sched::sleep_for(std::chrono::milliseconds{50});
  sched::exit((void*)(i + 1));
  gthread_log_fatal("cannot be here!");
  return NULL;
}

TEST(gthread_sched, timed_sleepy_threads) {
  sched::handle threads[k_num_tasks];

  std::cout << "timing this part" << std::endl;
  auto start = std::chrono::system_clock::now();
  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_tasks << " sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      threads[i] = sched::spawn(k_default_attr, sleepy_test_thread, (void*)i);
    }

    std::cout << "joining ALL the sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      void* ret;
      sched::join(&threads[i], &ret);
      EXPECT_EQ((uint64_t)ret, i + 1);
    }
  }
  std::cout << "gthread took "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now() - start)
                   .count()
            << " µs" << std::endl;
}

static thread_local std::atomic<uint64_t> shared_var;

void* tls_shared_var_test(void* _) { return (void*)shared_var.fetch_add(1); }

TEST(gthread_sched, no_tls_tasks) {
  sched::handle threads[k_num_tasks];
  std::vector<bool> counter_holes(k_num_tasks, false);

  for (int i = 0; i < 3; ++i) {
    shared_var = 0;

    std::cout << "spawning " << k_num_tasks << " threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      threads[i] = sched::spawn(k_light_attr, tls_shared_var_test, (void*)i);
    }

    std::cout << "joining ALL the threads" << std::endl;
    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      uint64_t ret;
      sched::join(&threads[i], (void**)&ret);
      ASSERT_GE(ret, 0);
      ASSERT_LT(ret, k_num_tasks);
      EXPECT_FALSE(counter_holes[ret]);
      counter_holes[ret] = true;
    }

    for (uint64_t i = 0; i < k_num_tasks; ++i) {
      EXPECT_TRUE(counter_holes[i]);
      counter_holes[i] = false;
    }
  }
}

TEST(gthread_sched, stress_fibers) {
  sched::handle threads[k_num_stress_tasks];
  std::vector<bool> counter_holes(k_num_stress_tasks, false);

  for (int i = 0; i < 3; ++i) {
    shared_var = 0;

    std::cout << "spawning " << k_num_stress_tasks << " threads" << std::endl;
    for (uint64_t i = 0; i < k_num_stress_tasks; ++i) {
      threads[i] = sched::spawn(k_light_attr, tls_shared_var_test, (void*)i);
    }

    std::cout << "joining ALL the threads" << std::endl;
    for (uint64_t i = 0; i < k_num_stress_tasks; ++i) {
      uint64_t ret;
      sched::join(&threads[i], (void**)&ret);
      ASSERT_GE(ret, 0);
      ASSERT_LT(ret, k_num_stress_tasks);
      EXPECT_FALSE(counter_holes[ret]);
      counter_holes[ret] = true;
    }

    for (uint64_t i = 0; i < k_num_stress_tasks; ++i) {
      EXPECT_TRUE(counter_holes[i]);
      counter_holes[i] = false;
    }
  }
}

TEST(gthread_sched, stress_threads) {
  sched::handle threads[k_num_stress_tasks];

  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_stress_tasks << " threads" << std::endl;
    for (uint64_t i = 0; i < k_num_stress_tasks; ++i) {
      threads[i] = sched::spawn(k_default_attr, tls_shared_var_test, (void*)i);
    }

    std::cout << "joining ALL the threads" << std::endl;
    for (uint64_t i = 0; i < k_num_stress_tasks; ++i) {
      uint64_t ret;
      sched::join(&threads[i], (void**)&ret);
      ASSERT_EQ(ret, 0);
    }
  }
}

void* sleeper(void* _) {
  sched::sleep_for(std::chrono::milliseconds{5});
  return nullptr;
}

TEST(gthread_sched, sleep_for) {
  std::cout << "sleeping in main" << std::endl;
  sched::sleep_for(std::chrono::milliseconds{5});
  {
    std::cout << "sleeping in k_light_attr task" << std::endl;
    auto h = sched::spawn(k_light_attr, sleeper, nullptr);
    sched::join(&h, nullptr);
  }
  {
    std::cout << "sleeping in k_default_attr task" << std::endl;
    auto h = sched::spawn(k_default_attr, sleeper, nullptr);
    sched::join(&h, nullptr);
  }
}

void* exit_quick(void* _) {
  std::cout << "exiting" << std::endl;
  return nullptr;
}

TEST(gthread_sched, detach_test_exit_quick) {
  auto h = sched::spawn(k_light_attr, exit_quick, nullptr);
  sched::sleep_for(std::chrono::milliseconds{5});
  sched::join(&h, nullptr);
}

void* exit_delay(void* _) {
  sched::sleep_for(std::chrono::milliseconds{5});
  std::cout << "exiting" << std::endl;
  return nullptr;
}

TEST(gthread_sched, detach_test_exit_delay) {
  auto h = sched::spawn(k_light_attr, exit_delay, nullptr);
  sched::join(&h, nullptr);
}
