#include "platform/alarm.h"

#include <ctime>
#include <system_error>

#include <signal.h>
#include <sys/time.h>

#include "util/compiler.h"

namespace gthread {

alarm::trap alarm::_trap{};
std::chrono::microseconds alarm::_interval{0};

thread_clock::time_point alarm::_last_alarm{};

/**
 * ITIMER_PROF is the right timer to use here since it measures stime+utime.
 * if an active thread is doing tons of syscalls, it should be penalized for
 * the time they take.
 */
constexpr int TIMER_TYPE = ITIMER_PROF;
constexpr int SIGNAL_TYPE = SIGPROF;

namespace {
std::chrono::microseconds from_itval(const struct itimerval& itval) {
  return std::chrono::microseconds{itval.it_value.tv_sec * 1000000 +
                                   itval.it_value.tv_usec};
}

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
  _last_alarm = thread_clock::now();
  _interval = usec;
}

void alarm::clear_interval() {
  set_interval_impl(std::chrono::microseconds{0});
}

std::chrono::microseconds alarm::reset() {
  struct itimerval it;
  if (branch_unexpected(getitimer(TIMER_TYPE, &it) < 0)) {
    throw std::system_error(errno, std::system_category(),
                            "error getting itimer");
  }

  auto remaining = from_itval(it);
  set_interval_internal(_interval);

  // if the time remaining on the itimer is greater than we expected, don't
  // freak out: just use the process time as an estimate
  auto start = _last_alarm;
  _last_alarm = thread_clock::now();
  if (remaining > _interval) {
    return std::chrono::duration_cast<std::chrono::microseconds>(_last_alarm -
                                                                 start);
  }

  return std::chrono::duration_cast<std::chrono::microseconds>(_interval -
                                                               remaining);
}

/**
 * signal handler for `SIGNAL_TYPE` alarms
 */
void alarm::alarm_handler(int signum) {
  sigset_t sigset;
  sigaddset(&sigset, SIGNAL_TYPE);
  sigprocmask(SIG_UNBLOCK, &sigset, NULL);

  if (_trap) _trap(_interval);
}

void alarm::ring_now() {
  auto elapsed = reset();
  if (_trap) _trap(elapsed);
}

void alarm::set_trap(const alarm::trap& t) {
  bool set_trap_now = !_trap;
  _trap = t;
  if (branch_unexpected(set_trap_now)) {
    signal(SIGNAL_TYPE, &alarm::alarm_handler);
  }
}

}  // namespace gthread
