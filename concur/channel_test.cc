#include "concur/channel.h"

#include "gtest/gtest.h"
#include "gthread.h"

using namespace gthread;

TEST(gthread_channel, make_channel) {
  auto c = make_channel<int>();
  { auto w = std::move(c.w); }
  c.r.read();
}

TEST(gthread_channel, read_and_write) {
  auto c = make_channel<int>();
  g a(
      [](auto&& r) {
        for (int i = 0; i < 10; ++i) {
          EXPECT_EQ(*r.read(), i);
        }
      },
      std::move(c.r));
  g b(
      [](auto&& w) {
        for (int i = 0; i < 10; ++i) {
          EXPECT_EQ(static_cast<bool>(w.write(i)), false);
        }
      },
      std::move(c.w));
  a.join();
  b.join();
}
