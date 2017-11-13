#include "util/function_marshall.h"

#include "gtest/gtest.h"

using namespace gthread;

int f(int y, const char* z) {
  return y;
}

TEST(gthread_function_marshall, instantiate) {
  auto m = make_function_marshall(f, 4, "bar");
  EXPECT_EQ(m(), 4);
}

TEST(gthread_function_marshall, move_only_to_lambda) {
  auto p = std::make_unique<int>(126);
  auto f = [](auto&& p) {
    return *p - 4;
  };
  auto m = make_function_marshall(f, std::move(p));
  EXPECT_EQ(m(), 122);
}
