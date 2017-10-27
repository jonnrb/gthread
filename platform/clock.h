#ifndef PLATFORM_CLOCK_H_
#define PLATFORM_CLOCK_H_

#include <chrono>
#include <ratio>

namespace gthread {
/**
 * time that increases while the process is running
 */
class thread_clock {
 public:
  // large enough for nanosecond values
  using rep = uint64_t;

  // thread clock MAY not be steady
  // see: https://linux.die.net/man/3/clock_gettime
  static constexpr bool is_steady = false;

  using period = std::nano;
  using duration = std::chrono::nanoseconds;

  using time_point = std::chrono::time_point<thread_clock>;

  static time_point now() noexcept;
  static duration resolution() noexcept;
};
};  // namespace gthread

#endif  // PLATFORM_CLOCK_H_
