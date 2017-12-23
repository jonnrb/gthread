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
  static void set_interval(Duration interval);

  /**
   * clears the interval if previously set
   */
  static void clear_interval();

  /**
   * sets the trap that gets sprung by the alarm
   */
  template <typename Function>
  static void set_trap(Function&& f);

 private:
  static void set_interval_impl(std::chrono::microseconds);

  static void alarm_handler(int signum);
  static void set_signal();

  static std::function<void()> _trap;
  static std::chrono::microseconds _interval;
};
}  // namespace gthread

#include "platform/alarm_impl.h"
