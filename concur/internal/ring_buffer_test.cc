#include "concur/internal/ring_buffer.h"

#include "gtest/gtest.h"

TEST(gthread_ring_buffer, fill_er_up) {
  constexpr auto ring_size = 128;
  constexpr auto how_sure_i_want_to_be = 1000;

  gthread::internal::ring_buffer<int, ring_size> ring{};

  for (int i = 0; i < how_sure_i_want_to_be; ++i) {
    for (auto j = 0; j < ring_size; ++j) {
      ASSERT_FALSE(ring.write(j));
    }

    for (auto j = 0; j < ring_size; ++j) {
      auto val = ring.read();
      ASSERT_TRUE(val);
      EXPECT_EQ(*val, j);
    }
  }
}
