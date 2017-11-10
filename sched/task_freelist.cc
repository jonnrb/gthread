#include "sched/task_freelist.h"

#include <mutex>

#include "sched/sched.h"

namespace gthread {
task* task_freelist::make_task(const attr& a) {
  task* t = nullptr;
  {
    std::lock_guard<sched> l(sched::get());
    if (_l.size() > 0) {
      t = _l.front();
      _l.pop_front();
    }
  }

  if (t != nullptr) {
    t->reset();
  } else {
    t = task::create(a);
  }
  return t;
}

void task_freelist::return_task(task* t) {
  {
    std::lock_guard<sched> l(sched::get());
    if (_l.size() < _max_size) {
      _l.push_back(t);
      return;
    }
  }

  t->destroy();
}
}  // namespace gthread
