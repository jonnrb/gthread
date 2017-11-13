#include "concur/internal/channel_window.h"

#include <mutex>

#include "absl/types/optional.h"
#include "concur/mutex.h"
#include "gtest/gtest.h"
#include "sched/task.h"

using namespace gthread;
using namespace gthread::internal;

constexpr auto n = 1E4;

void* w(void* arg) {
  auto* cw = static_cast<channel_window<int>*>(arg);
  for (auto i = 0; i < n; ++i) {
    auto v = i;
    cw->write(std::move(v));
  }
  return nullptr;
}

void* r(void* arg) {
  auto* cw = static_cast<channel_window<int>*>(arg);
  for (auto i = 0; i < n; ++i) {
    auto v = cw->read();
    EXPECT_EQ(*v, i);
  }
  return nullptr;
}

TEST(gthread_channel_window, quick_and_dirty) {
  channel_window<int> cw{};
  auto& s = sched::get();
  auto rh = s.spawn(k_light_attr, r, static_cast<void*>(&cw));
  auto wh = s.spawn(k_light_attr, w, static_cast<void*>(&cw));
  s.join(&rh, nullptr);
  s.join(&wh, nullptr);
}

struct shared_state {
  mutex mu;
  bool mail_flag;
  int mail;
};

void* w_mutex(void* arg) {
  auto* ss = (shared_state*)arg;
  for (auto i = 0; i < n; ++i) {
    while (true) {
      std::lock_guard<mutex> l{ss->mu};
      if (!ss->mail_flag) {
        ss->mail = i;
        ss->mail_flag = true;
        break;
      }
    }
  }
  return nullptr;
}

void* r_mutex(void* arg) {
  auto* ss = (shared_state*)arg;
  for (auto i = 0; i < n; ++i) {
    while (true) {
      std::lock_guard<mutex> l{ss->mu};
      if (ss->mail_flag) {
        EXPECT_EQ(ss->mail, i);
        ss->mail_flag = false;
        break;
      }
    }
  }
  return nullptr;
}

TEST(gthread_channel_window, mutex_comparison) {
  shared_state ss{};
  auto& s = sched::get();
  auto rh = s.spawn(k_light_attr, r_mutex, (void*)&ss);
  auto wh = s.spawn(k_light_attr, w_mutex, (void*)&ss);
  s.join(&rh, nullptr);
  s.join(&wh, nullptr);
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

#include <thread>

void* wait_and_close(void* arg) {
  auto* cw = static_cast<channel_window<int>*>(arg);
  auto& s = sched::get();
  std::this_thread::sleep_until(std::chrono::system_clock::now() +
                                std::chrono::milliseconds{5});
  std::cout << "foo" << std::endl;
  s.sleep_for(std::chrono::milliseconds{5});
  std::cout << "closing" << std::endl;
  cw->close();
  return nullptr;
}

TEST(gthread_channel_window, write_then_close) {
  channel_window<int> cw{};
  auto& s = sched::get();
  auto h = s.spawn(k_light_attr, wait_and_close, static_cast<void*>(&cw));
  std::cout << "writing" << std::endl;
  auto val = cw.write(4);
  s.join(&h, nullptr);
  ASSERT_EQ(*val, 4);
}

TEST(gthread_channel_window, read_then_close) {
  channel_window<int> cw{};
  auto& s = sched::get();
  auto h = s.spawn(k_light_attr, wait_and_close, static_cast<void*>(&cw));
  std::cout << "reading" << std::endl;
  auto val = cw.read();
  s.join(&h, nullptr);
  ASSERT_EQ(static_cast<bool>(val), false);
}
