#include "platform/clock.h"

#include <cerrno>
#include <ctime>

#include "util/compiler.h"

namespace gthread {

namespace {
uint64_t posix_time_from_type(clockid_t c) {
  struct timespec t;
  clock_gettime(c, &t);
  return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}

uint64_t posix_time_resolution_from_type(clockid_t c) {
  struct timespec t;
  clock_getres(c, &t);
  return t.tv_sec * 1000 * 1000 * 1000 + t.tv_nsec;
}
}  // namespace

thread_clock::time_point thread_clock::now() noexcept {
  return thread_clock::time_point(
      std::chrono::nanoseconds{posix_time_from_type(CLOCK_THREAD_CPUTIME_ID)});
}

thread_clock::duration thread_clock::resolution() noexcept {
  return std::chrono::nanoseconds{
      posix_time_resolution_from_type(CLOCK_THREAD_CPUTIME_ID)};
}

}  // namespace gthread
