#ifndef UTIL_COMPILER_H_
#define UTIL_COMPILER_H_

/**
 * these macros hint to the compiler if a branch is expected or unexpected to
 * optimize the ordering of the machine code
 */
#define branch_expected(expr) __builtin_expect(!!(expr), 1)
#define branch_unexpected(expr) __builtin_expect(expr, 0)

#ifndef offsetof
#define offsetof(type, member) __builtin_offsetof(type, member)
#endif

#define typeof(type) __typeof__(type)

/**
 * this macro subtracts off the offset of |member| within |type| from |ptr|,
 * allowing you to easily get a pointer to the containing struct of |ptr|
 *
 *           ┌─────────────┬──────────┬──────┐
 * |type| -> │ XXXXXXXXXXX │ |member| │ YYYY │
 *           └─────────────┴──────────┴──────┘
 *           ^             ^
 *        returns        |ptr|
 */
#define container_of(ptr, type, member)                \
  ({                                                   \
    const typeof(((type *)0)->member) *__mptr =        \
        (const typeof(((type *)0)->member) *)(__mptr); \
    (type *)((char *)ptr - offsetof(type, member));    \
  })

#endif  // UTIL_COMPILER_H_
