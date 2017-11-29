#include "concur/channel.h"

#include "gtest/gtest.h"
#include "gthread.h"

using namespace gthread;

TEST(gthread_channel, read_from_closed) {
  auto c = make_channel<int>();
  { auto write = std::move(c.write); }
  c.read();
}

TEST(gthread_channel, write_to_closed) {
  auto c = make_channel<int>();
  { auto read = std::move(c.read); }
  c.write(5);
}

TEST(gthread_channel, read_and_write) {
  auto c = make_channel<int>();

  g a([read = std::move(c.read)]() {
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(*read(), i);
    }
  });

  g b([write = std::move(c.write)]() {
    for (int i = 0; i < 10; ++i) {
      EXPECT_EQ(static_cast<bool>(write(i)), false);
    }
  });

  a.join();
  b.join();
}

TEST(gthread_channel, fill_and_drain) {
  constexpr size_t size = 8;
  auto c = gthread::make_buffered_channel<int, size>();

  for (auto i = 0; i < size; ++i) {
    EXPECT_EQ(static_cast<bool>(c.write(i)), false);
  }

  { auto writer = std::move(c.write); }

  for (auto i = 0; i < size; ++i) {
    auto v = c.read();
    ASSERT_EQ(static_cast<bool>(v), true);
    EXPECT_EQ(*v, i);
  }
}
