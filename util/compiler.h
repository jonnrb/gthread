#ifndef UTIL_COMPILER_H_
#define UTIL_COMPILER_H_

#define branch_expected(expr) __builtin_expect(!!(expr), 1)
#define branch_unexpected(expr) __builtin_expect(expr, 0)

#endif  // UTIL_COMPILER_H_
