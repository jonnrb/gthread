/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to.h
 * info: inline assembler for switching to and spawning new contexts
 */

#ifndef ARCH_SWITCH_TO_H_
#define ARCH_SWITCH_TO_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * non-volatile registers must be saved by switch_to().
 * MS ABI (not used): https: *msdn.microsoft.com/en-us/library/6t169e9c.aspx
 * SysV ABI (Linux, XNU):
 * https://software.intel.com/sites/default/files/article/402129/mpx-linux64-abi.pdf
 */
typedef struct _gthread_saved_ctx {
  uint64_t rbx;
  uint64_t rbp;
  uint64_t rsp;
  uint64_t r12;
  uint64_t r13;
  uint64_t r14;
  uint64_t r15;
} gthread_saved_ctx_t;

/**
 * saves the current context into |from| and loads the context stored in |to|.
 * when `gthread_switch_to()` returns, control will be at the context stored
 * in |to|.
 *
 * if an execution context is saved with `gthread_switch_to()` and is returned
 * control with `gthread_switch_to()` by another context, the saved context
 * will resume control after the point where it called `gthread_switch_to()`.
 */
void gthread_switch_to(gthread_saved_ctx_t* from, gthread_saved_ctx_t* to);

/**
 * saves the current context to |self_ctx| and runs the function |entry| with
 * the specified argument (`entry(arg)`).
 */
void gthread_switch_to_and_spawn(gthread_saved_ctx_t* self_ctx,
                                 gthread_saved_ctx_t* ret_ctx, void* stack,
                                 void (*entry)(void*), void* arg);

#ifdef __cplusplus
};
#endif

#endif  // ARCH_SWITCH_TO_H_
