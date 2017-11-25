// NOTE: link against the libmy_pthread.a static library
#ifndef MY_MALLOC_H_
#define MY_MALLOC_H_

#include "my_malloc/mymalloc.h"
#include "sched/task.h"

#define malloc(size) mymalloc((size), gthread_task_current())
#define free(ptr) myfree((ptr), gthread_task_current())

#endif  // MY_MALLOC_H_
