#include "platform/clock.h"

#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using float_sec = std::chrono::duration<float>;

template <typename T>
float_sec sleep_for(T t) {
  using namespace std::chrono;
  auto start = high_resolution_clock::now();
  std::this_thread::sleep_for(t);
  float_sec fsec = high_resolution_clock::now() - start;
  std::cout << "slept for " << (fsec.count() * 1E3) << " ms" << std::endl;
  return fsec;
}

int main() {
  using namespace gthread;
  using milliseconds = std::chrono::milliseconds;

  auto last = thread_clock::now();
  std::cout << "sizeof(thread_clock::now()) => " << sizeof(last) << std::endl;
  for (int i = 0; i < 10; ++i) {
    auto sleep_time = sleep_for(milliseconds{10});
    auto t = thread_clock::now();
    float_sec elapsed = t - last;
    assert(sleep_time > elapsed);
    std::cout << "elapsed: " << (elapsed.count() * 1E3) << "ms" << std::endl;
    last = t;
  }

  return 0;
}
