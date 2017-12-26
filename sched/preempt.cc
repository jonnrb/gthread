#include "sched/preempt.h"

#include "platform/alarm.h"
#include "sched/sched_node.h"
#include "util/compiler.h"

namespace gthread {
namespace {
std::atomic_flag g_alarm_armed{false};
}  // namespace

void enable_timer_preemption() {
  if (!g_alarm_armed.test_and_set()) {
    /**
     * this right here is the function that gets run on the alarm's tick in
     * each kernel-managed thread
     */
    alarm::set_trap([]() mt_no_analysis {
      // if there is no `sched_node` on the current kernel-managed thread,
      // there is nothing to do
      if (sched_node::current() == nullptr) return;

      // check if the current execution context does not wish to be preempted
      auto& mu = preempt_mutex::get();
      if (static_cast<bool>(mu)) return;

      // apply the preempt lock to prevent trampling (and assert that no other
      // concurrent execution context sets the `no_preempt_flag`)
      std::lock_guard<preempt_mutex> l(mu);
      mu.node().yield();
    });
  }
  alarm::set_interval(preempt_interval);
}

void disable_timer_preemption() { alarm::clear_interval(); }

preempt_mutex& preempt_mutex::get() {
  static preempt_mutex mu;
  return mu;
}

void preempt_mutex::lock() {
  auto* cur = task::current();
  auto expected = false;
  auto succ[[maybe_unused]] = cur->no_preempt_flag.compare_exchange_strong(
      expected, true, std::memory_order_acquire);
  assert(succ);
}

void preempt_mutex::unlock() {
  auto* cur = task::current();
  auto expected = true;
  auto succ[[maybe_unused]] = cur->no_preempt_flag.compare_exchange_strong(
      expected, false, std::memory_order_release);
  assert(succ);
}

sched_node& preempt_mutex::node() const {
  auto* node = sched_node::current();
  assert(node != nullptr);
  return *node;
}
}  // namespace gthread
