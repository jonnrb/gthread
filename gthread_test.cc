#include "gthread.h"

#include "gtest/gtest.h"

TEST(gthread, g_sanity_thread) {
  int x = 4;
  gthread::g([&x]() { ++x; }).join();
  EXPECT_EQ(x, 5);
}

TEST(gthread, g_move_only_arg) {
  auto x = std::make_unique<int>(4);
  gthread::g([](auto&& ux) { EXPECT_EQ(*ux, 4); }, std::move(x));
}
