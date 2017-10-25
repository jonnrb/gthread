/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//arch/switch_to.S
 * info: AT&T assembly for switching to and spawning new contexts
 */

.global gthread_switch_to
gthread_switch_to:
  test %rdi, %rdi
  jz 1f
  mov %rbx, 0x00(%rdi)
  mov %rsp, 0x08(%rdi)
  mov %rbp, 0x10(%rdi)
  mov %r12, 0x18(%rdi)
  mov %r13, 0x20(%rdi)
  mov %r14, 0x28(%rdi)
  mov %r15, 0x30(%rdi)
1:
  mov 0x00(%rsi), %rbx
  mov 0x08(%rsi), %rsp
  mov 0x10(%rsi), %rbp
  mov 0x18(%rsi), %r12
  mov 0x20(%rsi), %r13
  mov 0x28(%rsi), %r14
  mov 0x30(%rsi), %r15
  ret

.global gthread_switch_to_and_spawn
gthread_switch_to_and_spawn:
  test %rdi, %rdi
  jz 1f
  mov %rbx, 0x00(%rdi)
  mov %rsp, 0x08(%rdi)
  mov %rbp, 0x10(%rdi)
  mov %r12, 0x18(%rdi)
  mov %r13, 0x20(%rdi)
  mov %r14, 0x28(%rdi)
  mov %r15, 0x30(%rdi)
1:
  lea -0x8(%rsi), %rsp # sets the stack pointer to 16 bytes before |stack|
  mov $0x0, %rbp
  movl $0xdeadbeef, (%rsp)
  mov %rcx, %rdi # moves 4th parameter |arg| to the 1st parameter of |entry|
  jmp *%rdx       # jumps to |entry|
