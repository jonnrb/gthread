#include "gthread.h"

#include <atomic>
#include <chrono>

#include "gtest/gtest.h"

TEST(gthread, g_sanity_thread) {
  int x = 4;
  gthread::g([&x]() { ++x; }).join();
  EXPECT_EQ(x, 5);
}

TEST(gthread, g_move_only_arg) {
  auto x = std::make_unique<int>(4);
  gthread::g([](auto&& ux) { EXPECT_EQ(*ux, 4); }, std::move(x)).join();
}

TEST(gthread, g_join) {
  gthread::g([]() {
      std::cout << "hi" << std::endl;
  }).join();
}

TEST(gthread, g_detach) {
  int i = 0;
  std::atomic<bool> done = false;
  gthread::g([&i, &done]() {
      gthread::g([&i, &done]() {
          i = 42;
          done.store(true);
      });
  }).detach();
  while (!done.load()) gthread::self::yield();
  EXPECT_EQ(i, 42);
}
