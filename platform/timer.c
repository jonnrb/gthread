/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/timer.c
 * info: platform timer utilities
 */

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))
#include "platform/timer_posix.c"
#else
#error ":("
#endif
