#include "sched/internal/task_freelist.h"

#include <mutex>

namespace gthread {
namespace internal {
namespace {
template <typename Iterable>
task* find_task(Iterable* iterable, const attr& a) {
  for (auto i = iterable->begin(); i != iterable->end(); ++i) {
    if (a.stack.size == (*i)->stack_size() && a.alloc_tls == (*i)->has_tls()) {
      task* t = *i;
      iterable->erase(i);
      return t;
    }
  }

  return nullptr;
}

template <typename Iterable>
void insert_task(Iterable* iterable, task* t) {
  auto i = iterable->begin();
  for (; i != iterable->end(); ++i) {
    if (t->stack_size() >= (*i)->stack_size() &&
        (t->has_tls() || !(*i)->has_tls())) {
      iterable->insert(i, t);
      return;
    }
  }
  iterable->insert(i, t);
}
}  // namespace

using guard = std::lock_guard<std::mutex>;
using unique_lock = std::unique_lock<std::mutex>;

task* task_freelist::make_task(const attr& a) {
  task* t = nullptr;
  {
    guard l(_mu);
    if (_l.size() > 0) {
      t = find_task(&_l, a);
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
    guard l(_mu);
    if (_l.size() < _max_size) {
      insert_task(&_l, t);
      return;
    }
  }

  t->destroy();
}
}  // namespace internal
}  // namespace gthread
