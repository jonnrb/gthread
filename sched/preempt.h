#pragma once

namespace gthread {
/**
 * enables task preemption by a timer for the entire process
 */
void enable_timer_preemption();

/**
 * disables task preemption by a timer for the entire process
 */
void disable_timer_preemption();
}  // namespace gthread
