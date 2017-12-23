#pragma once

#include <list>
#include <mutex>

#include "sched/task.h"
#include "sched/task_attr.h"

namespace gthread {
namespace internal {
class task_freelist {
 public:
  explicit task_freelist(unsigned max_size) : _max_size(max_size) {}

  task* make_task(const attr& a);

  void return_task(task* t);

  void return_task_from_scheduler(task* t);

 private:
  std::mutex _mu;
  std::list<task*> _l;
  unsigned _max_size;
};
}  // namespace internal
}  // namespace gthread
