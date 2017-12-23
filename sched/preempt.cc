#include "sched/preempt.h"

#include "platform/alarm.h"
#include "sched/sched_node.h"

namespace gthread {
namespace {
std::atomic_flag g_alarm_armed{false};
}

void enable_timer_preemption() {
  constexpr auto interval = std::chrono::milliseconds{50};

  if (!g_alarm_armed.test_and_set()) {
    /**
     * this right here is the function that gets run on the alarm's tick in
     * each kernel-managed thread
     */
    alarm::set_trap([]() {
      auto* node = sched_node::current();
      if (node != nullptr) node->yield();
    });
  }
  alarm::set_interval(interval);
}

void disable_timer_preemption() { alarm::clear_interval(); }
}  // namespace gthread
