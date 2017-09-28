/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to.c
 * info: inline assembler for switching to and spawning new contexts
 */

#include "switch_to.h"

// very simple switch_to() like the linux kernel do.
void gthread_switch_to(gthread_saved_ctx_t* from, gthread_saved_ctx_t* to) {
  __asm__ __volatile__(
      "  test %0, %0        \n"  // if |from| is NULL don't save ctx.
      "  jz no_save         \n"
      "  mov %%rbx, 0x00(%0)\n"  // move non-volatile registers to %[from].
      "  mov %%rbp, 0x08(%0)\n"
      "  mov %%rsp, 0x10(%0)\n"  // saving %rsp means a `gthread_switch_to()`
                                 // back to the saved ctx will return back into
                                 // the original caller.
      "  mov %%r12, 0x18(%0)\n"
      "  mov %%r13, 0x20(%0)\n"
      "  mov %%r14, 0x28(%0)\n"
      "  mov %%r15, 0x30(%0)\n"
      "no_save:             \n"
      : "=D"(from)  // output
      :             // no inputs
      :             // shhh this clobbers everything...
  );

  // TODO(jonnrb): possibly do signal masking here?

  // move saved registers from %[to]
  __asm__ __volatile__(
      "  mov 0x00(%0), %%rbx\n"
      "  mov 0x08(%0), %%rbp\n"
      "  mov 0x10(%0), %%rsp\n"  // moving %rsp changes the effect of ret when
                                 // resumed (returns on new ctx).
      "  mov 0x18(%0), %%r12\n"
      "  mov 0x20(%0), %%r13\n"
      "  mov 0x28(%0), %%r14\n"
      "  mov 0x30(%0), %%r15\n"
      :          // no outputs
      : "S"(to)  // input
      :          // shhh this clobbers everything...
  );
}

void gthread_switch_to_and_spawn(gthread_saved_ctx_t* self_ctx,
                                 gthread_saved_ctx_t* ret_ctx, void* stack,
                                 void (*entry)(void*), void* arg) {
  // save the current context.
  __asm__ __volatile__(
      "mov %%rbx, 0x00(%0)\n"
      "mov %%rbp, 0x08(%0)\n"
      "mov %%rsp, 0x10(%0)\n"  // switching to |self_ctx| will resume at the
                               // call point of `gthread_switch_to_and_spawn()`
      "mov %%r12, 0x18(%0)\n"
      "mov %%r13, 0x20(%0)\n"
      "mov %%r14, 0x28(%0)\n"
      "mov %%r15, 0x30(%0)\n"
      : "=D"(self_ctx)  // output
      :                 // no inputs
      :                 // shhh this clobbers everything...
  );

  // save |ret_ctx| to the bottom of the stack.
  gthread_saved_ctx_t** rsp = ((gthread_saved_ctx_t**)stack) - 1;
  *rsp = ret_ctx;
  --rsp;

  __asm__ __volatile__(
      "mov %0, %%rsp          \n"  // change the stack pointer to |stack|
      "call *%1               \n"  // call `entry(arg)`
      "                       \n"
      "mov 0x8(%%rsp), %%rsi  \n"  // move saved |ret_ctx| to param2 (SysV ABI)
      "movq     $0, %%rdi     \n"  // move NULL to param1 (SysV ABI)
      "call _gthread_switch_to\n"
      :                                 // no output
      : "r"(rsp), "r"(entry), "D"(arg)  // inputs
      : "rsp", "rsi"                    // clobbers
  );
}
