#include "concur/internal/channel_window.h"

#include <mutex>

#include "concur/mutex.h"
#include "gtest/gtest.h"
#include "gthread.h"
#include "sched/task.h"

using namespace gthread;
using namespace gthread::internal;

TEST(gthread_channel_window, quick_and_dirty) {
  constexpr auto n = 1E4;
  channel_window<int> cw{};

  gthread::g reader([&cw]() {
    for (auto i = 0; i < n; ++i) {
      auto v = cw.read();
      EXPECT_EQ(*v, i);
    }
  });

  gthread::g writer([&cw]() {
    for (auto i = 0; i < n; ++i) {
      cw.write(i);
    }
  });

  reader.join();
  writer.join();
}

TEST(gthread_channel_window, mutex_comparison) {
  constexpr auto n = 1E4;
  mutex mu;
  bool mail_flag;
  int mail;

  gthread::g reader([&mu, &mail_flag, &mail]() {
    for (auto i = 0; i < n; ++i) {
      while (true) {
        std::lock_guard<mutex> l{mu};
        if (mail_flag) {
          EXPECT_EQ(mail, i);
          mail_flag = false;
          break;
        }
      }
    }
  });

  gthread::g writer([&mu, &mail_flag, &mail]() {
    for (auto i = 0; i < n; ++i) {
      while (true) {
        std::lock_guard<mutex> l{mu};
        if (!mail_flag) {
          mail = i;
          mail_flag = true;
          break;
        }
      }
    }
  });

  reader.join();
  writer.join();
}

TEST(gthread_channel_window, close_then_write) {
  channel_window<int> cw{};
  cw.close();
  auto val = cw.write(4);
  ASSERT_EQ(*val, 4);
}

TEST(gthread_channel_window, close_then_read) {
  channel_window<int> cw{};
  cw.close();
  auto val = cw.read();
  ASSERT_EQ(static_cast<bool>(val), false);
}

void wait_and_close(channel_window<int>& cw) {
  gthread::self::sleep_for(std::chrono::milliseconds{5});
  std::cout << "closing" << std::endl;
  cw.close();
}

TEST(gthread_channel_window, write_then_close) {
  channel_window<int> cw;
  gthread::g closer(wait_and_close, std::ref(cw));
  std::cout << "writing" << std::endl;
  auto val = cw.write(4);
  closer.join();
  ASSERT_EQ(*val, 4);
}

TEST(gthread_channel_window, read_then_close) {
  channel_window<int> cw;
  gthread::g closer(wait_and_close, std::ref(cw));
  std::cout << "reading" << std::endl;
  auto val = cw.read();
  closer.join();
  ASSERT_EQ(static_cast<bool>(val), false);
}
