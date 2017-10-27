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
