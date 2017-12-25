#include "concur/internal/waiter.h"

#include "gtest/gtest.h"
#include "gthread.h"

TEST(gthread_waiter, quick_switch) {
  gthread::internal::waiter w;

  gthread::g([&w]() {
    while (!w.unpark()) {
    }
    std::cout << "unparking" << std::endl;
  });

  std::cout << "parking" << std::endl;
  while (!w.park()) {
  }
  std::cout << "unparked" << std::endl;
}

TEST(gthread_waiter, ping_pong) {
  constexpr auto k_ping_pong = 1E6;
  gthread::internal::waiter w;

  gthread::g([&w]() {
    for (int i = 0; i < k_ping_pong; ++i) {
      while (!w.swap()) {
      }
    }
    while (!w.unpark()) {
    }
  });

  gthread::g([&w]() {
    while (!w.park()) {
    }
    for (int i = 0; i < k_ping_pong; ++i) {
      while (!w.swap()) {
      }
    }
  })
      .join();
}
