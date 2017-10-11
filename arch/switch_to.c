/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to.c
 * info: inline assembler for switching to and spawning new contexts
 */

#include "switch_to.h"

// very simple switch_to() like the linux kernel do.
void __attribute__((noinline))
gthread_switch_to(gthread_saved_ctx_t* from, gthread_saved_ctx_t* to) {
  // intentionally does not list clobbers. if clobbers are listed here, the
  // compiler will push each and every register clobbered to the stack,
  // effectively saving the context to the stack, which is not what we want.
  // some finesse is required, but this cuts the memory ops in half.
  __asm__ __volatile__(
      "  test %0, %0        \n"  // if |from| is NULL don't save ctx.
      "  jz 1f              \n"
      "  mov %%rbx, 0x00(%0)\n"  // move non-volatile registers to %[from].
      "  mov %%rbp, 0x08(%0)\n"
      "  mov %%rsp, 0x10(%0)\n"  // saving %rsp means a `gthread_switch_to()`
                                 // back to the saved ctx will return back into
                                 // the original caller.
      "  mov %%r12, 0x18(%0)\n"
      "  mov %%r13, 0x20(%0)\n"
      "  mov %%r14, 0x28(%0)\n"
      "  mov %%r15, 0x30(%0)\n"
      "1:                   \n"
      :
      : "D"(from)  // (%rdi will be the first arg, hint hint compiler)
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
  );
}

void __attribute__((noinline))
gthread_switch_to_and_spawn(gthread_saved_ctx_t* self_ctx,
                            gthread_saved_ctx_t* ret_ctx, void* stack,
                            void (*entry)(void*), void* arg) {
  // save the current context. does not list clobbers: see `gthread_switch_to()`
  __asm__ __volatile__(
      "mov %%rbx, 0x00(%0)\n"
      "mov %%rbp, 0x08(%0)\n"
      "mov %%rsp, 0x10(%0)\n"  // switching to |self_ctx| will resume at the
                               // call point of `gthread_switch_to_and_spawn()`
      "mov %%r12, 0x18(%0)\n"
      "mov %%r13, 0x20(%0)\n"
      "mov %%r14, 0x28(%0)\n"
      "mov %%r15, 0x30(%0)\n"
      :                // no outputs
      : "D"(self_ctx)  // input
  );

  // pray that we may return from this inline asm horror
  __asm__ __volatile__(
      "mov %1, %%rsp \n"  // change the stack pointer to |stack|
      "mov $0, %%rbp \n"  // tells {g,ll}db that this is the bottom stack frame
      "pushq %%rsi   \n"  // push the return context to the new stack
      "pushq %0      \n"  // make stack 16-byte aligned as is the ritual
      "call *%2      \n"  // call `entry(arg)` (rdi is prepopulated)
      "              \n"
      "popq %%rax    \n"
      "popq %%rsi    \n"  // move saved |ret_ctx| to param2 (SysV ABI)
      "movq $0, %%rdi\n"  // move NULL to param1 (SysV ABI)
      "jmp *%%rax    \n"  // no coming back
      :
      : "r"(gthread_switch_to), "r"(stack), "r"(entry), "D"(arg), "S"(ret_ctx)
      // inputs
  );
}
