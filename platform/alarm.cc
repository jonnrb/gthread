#include "platform/alarm.h"

#include <ctime>
#include <system_error>

#include <signal.h>
#include <sys/time.h>

#include "util/compiler.h"

namespace gthread {
std::function<void()> alarm::_trap{};
std::chrono::microseconds alarm::_interval{0};

/**
 * ITIMER_PROF is the right timer to use here since it measures stime+utime.
 * if an active thread is doing tons of syscalls, it should be penalized for
 * the time they take.
 */
constexpr auto TIMER_TYPE = ITIMER_PROF;
constexpr auto SIGNAL_TYPE = SIGPROF;

namespace {
void set_interval_internal(std::chrono::microseconds usec) {
  struct itimerval itval;

  itval.it_value.tv_sec = usec.count() / 1000000;
  itval.it_value.tv_usec = usec.count() % 1000000;
  itval.it_interval.tv_sec = usec.count() / 1000000;
  itval.it_interval.tv_usec = usec.count() % 1000000;

  if (branch_unexpected(setitimer(TIMER_TYPE, &itval, NULL))) {
    throw std::system_error(errno, std::system_category(),
                            "error setting itimer");
  }
}
}  // namespace

void alarm::set_interval_impl(std::chrono::microseconds usec) {
  set_interval_internal(usec);
  _interval = usec;
}

void alarm::clear_interval() {
  set_interval_impl(std::chrono::microseconds{0});
}

/**
 * signal handler for `SIGNAL_TYPE` alarms
 */
void alarm::alarm_handler(int signum) {
  sigset_t sigset;
  sigaddset(&sigset, SIGNAL_TYPE);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);

  if (_trap) _trap();
}

void alarm::set_signal() { signal(SIGNAL_TYPE, &alarm::alarm_handler); }
}  // namespace gthread
