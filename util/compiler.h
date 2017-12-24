#pragma once

/**
 * these macros hint to the compiler if a branch is expected or unexpected to
 * optimize the ordering of the machine code
 */
#define branch_expected(expr) __builtin_expect(!!(expr), 1)
#define branch_unexpected(expr) __builtin_expect(expr, 0)

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#ifndef typeof
#define typeof(type) __typeof__(type)
#endif

#define unused_value __attribute__((unused))

// try to detect if thread safety attributes are supported
// https://clang.llvm.org/docs/ThreadSafetyAnalysis.html
#if defined(__clang__) && !defined(SWIG)
#define gthread_thread_safety_attr(x) __attribute__((x))
#else
#define gthread_thread_safety_attr(x)
#endif

#define mt_no_analysis gthread_thread_safety_attr(no_thread_safety_analysis)

#define mt_capability(cap) gthread_thread_safety_attr(capability(cap))

#define mt_scoped_capability gthread_thread_safety_attr(scoped_lockable)

#define mt_guarded_by(mu) gthread_thread_safety_attr(guarded_by(mu))

#define mt_acquire(...) \
  gthread_thread_safety_attr(acquire_capability(__VA_ARGS__))

#define mt_try_acquire(...) \
  gthread_thread_safety_attr(try_acquire_capability(__VA_ARGS__))

#define mt_acquired_after(...) \
  gthread_thread_safety_attr(acquired_after(__VA_ARGS__))

#define mt_release(...) \
  gthread_thread_safety_attr(release_capability(__VA_ARGS__))

#define mt_locks_required(...) \
  gthread_thread_safety_attr(exclusive_locks_required(__VA_ARGS__))

#define mt_locks_excluded(...) \
  gthread_thread_safety_attr(locks_excluded(__VA_ARGS__))

#define mt_return_capability(mu) gthread_thread_safety_attr(lock_returned(mu))
