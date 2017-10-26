# TODO items

- Review atomic usage for memory ordering correctness (I was very hasty in
  adding C++ atomics without thinking through the membars).

- Add noop TLS implementation. Switching the thread pointer MSR is slow since
  it involves a syscall. There should be a noop version that still allows
  `task::current()` to function correctly but not involve any syscalls on
  the `task::switch_to()` hot path.
