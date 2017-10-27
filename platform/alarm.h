#pragma once

#include <chrono>
#include <functional>

#include "platform/clock.h"

namespace gthread {

/**
 * an alarm that sends a signal to the process at a predefined interval
 */
class alarm {
 public:
  /**
   * sets the interval for an alarm to be rung. this is global to the process.
   *
   * `set_trap` should be called first. an interval of 0 will disable the timer
   * and does the same thing as `clear_interval()`
   *
   * |duration| is compatible with `std::chrono::duration` and cannot be
   * expected to have better than microsecond resolution
   */
  template <typename Duration>
  static void set_interval(Duration interval) {
    set_interval_impl(
        std::chrono::duration_cast<std::chrono::microseconds>(interval));
  }

  /**
   * clears the interval if previously set
   */
  static void clear_interval();

  /*
   * resets the interval and returns the elapsed time
   */
  static std::chrono::microseconds reset();

  /**
   * triggers the alarm immediately and on the current stack
   */
  static void ring_now();

  using trap = std::function<void(std::chrono::microseconds)>;

  /**
   * sets the trap that gets sprung by the alarm
   */
  static void set_trap(const trap& t);

 private:
  static void alarm_handler(int signum);

  static void set_interval_impl(std::chrono::microseconds);

  static trap _trap;
  static std::chrono::microseconds _interval;

  static thread_clock::time_point _last_alarm;
};

}  // namespace gthread
