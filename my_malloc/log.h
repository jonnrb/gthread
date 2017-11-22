#ifndef MY_MALLOC_LOG_H_
#define MY_MALLOC_LOG_H_

#ifdef LOG_DEBUG
#define debug_impl(fmt, ...) \
  fprintf(stderr, "DEBUG [%s:%d] " fmt "%s\n", __FILE__, __LINE__, __VA_ARGS__)
#define debug(...) debug_impl(__VA_ARGS__, "")
#else
#define debug(...)
#endif

#endif
