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
      !_waiter.compare_exchange_strong(parked, nullptr,
                                       std::memory_order_release)) {
    return false;
  }

  assert(parked->run_state == task::WAITING);

  auto& pmu = preempt_mutex::get();
  std::lock_guard<preempt_mutex> l(pmu);
  parked->run_state = task::SUSPENDED;
  pmu.node().schedule(parked);

  return true;
}

bool waiter::swap() {
  auto* parked = _waiter.load();
  auto* current = task::current();

  if (parked == nullptr || parked->run_state != task::WAITING ||
      !_waiter.compare_exchange_strong(parked, current,
                                       std::memory_order_seq_cst)) {
    return false;
  }

  auto& pmu = preempt_mutex::get();
  std::lock_guard<preempt_mutex> l(pmu);
  current->run_state = task::WAITING;
  pmu.node().switch_to(parked);

  return true;
}
}  // namespace internal
}  // namespace gthread
