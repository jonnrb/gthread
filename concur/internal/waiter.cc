#include "concur/internal/waiter.h"

#include <cassert>
#include <mutex>

#include "sched/sched.h"

namespace gthread {
namespace internal {
bool waiter::park() {
  return park_if([]() { return true; });
}

bool waiter::unpark() {
  auto* parked = _waiter.load();
  if (parked == nullptr || parked->run_state != task::WAITING ||
      !_waiter.compare_exchange_strong(parked, nullptr)) {
    return false;
  }

  assert(parked->run_state == task::WAITING);

  auto& s = sched::get();
  {
    std::lock_guard<sched> l(s);
    parked->run_state = task::SUSPENDED;
    s.runqueue_push(parked);
  }

  return true;
}

bool waiter::swap() {
  auto* parked = _waiter.load();
  auto* current = task::current();

  if (parked == nullptr || parked->run_state != task::WAITING ||
      !_waiter.compare_exchange_strong(parked, current)) {
    return false;
  }

  auto& s = sched::get();
  {
    std::lock_guard<sched> l(s);
    current->run_state = task::WAITING;
    parked->switch_to();
  }

  return true;
}
}  // namespace internal
}  // namespace gthread