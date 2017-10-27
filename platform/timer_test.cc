#include "platform/timer.h"

#include <chrono>
#include <iostream>
#include <thread>

int main() {
  using clock = std::chrono::system_clock;
  clock::duration timer_duration;
  {
    gthread::timer<clock> t(
        [&timer_duration](clock::duration d) { timer_duration = d; });
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
  }
  std::cout << "timer took "
            << std::chrono::duration_cast<std::chrono::duration<float>>(
                   timer_duration)
                   .count()
            << " s" << std::endl;
}
