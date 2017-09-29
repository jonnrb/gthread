/**
 * author: JonNRb <jonbetti@gmail.com>
 * license: MIT
 * file: @gthread//platform/memory.c
 * info: text-includes platform-specific memory stuff (for make easy gnu)
 */

#if defined(__APPLE__) && defined(__MACH__)
#include "platform/memory_macos.c"
#elif defined(__linux__)
#include "platform/memory_linux.c"
#else
#error ":("
#endif
