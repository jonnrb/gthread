#pragma once

#include "concur/internal/channel_window.h"
#include "sched/sched.h"
#include "sched/task.h"

namespace gthread {
namespace internal {
template <typename T>
void channel_window<T>::close() {
  sched& s = sched::get();
  if (_closed) return;
  std::unique_lock<sched> l{s};
  _closed = true;
  if (_waiter != nullptr) {
    _hot_potato = std::move(l);
    s.runqueue_push(task::current());
    task* waiter = _waiter;
    _waiter = nullptr;
    _reader = nullptr;
    waiter->switch_to();
  }
}

template <typename T>
absl::optional<T> channel_window<T>::read() {
  sched& s = sched::get();
  absl::optional<T> ret;

  if (!_closed) {
    std::unique_lock<sched> l{s};
    if (_closed) {
      return ret;
    }
    if (_waiter == nullptr) {
      // read called before write
      _waiter = task::current();
      _waiter->run_state = task::WAITING;
      _reader = &ret;
      l.unlock();
      s.yield();

      assert(static_cast<bool>(_hot_potato) && _waiter == nullptr &&
             _reader == nullptr);
      l = std::move(_hot_potato);
    } else {
      // read called after write
      task* writer = _waiter;
      _reader = &ret;
      _waiter = task::current();
      _waiter->run_state = task::WAITING;
      _hot_potato = std::move(l);
      writer->switch_to();
    }
  }

  return ret;
}

template <typename T>
template <typename U>
absl::optional<T> channel_window<T>::write(U&& t) {
  sched& s = sched::get();

  if (_closed) {
    return std::move(t);
  }

  std::unique_lock<sched> l{s};

  if (_closed) {
    return std::move(t);
  }

  if (_waiter == nullptr) {
    // write called before read
    task* current = task::current();
    _waiter = current;
    _waiter->run_state = task::WAITING;
    l.unlock();
    s.yield();

    // the channel may be closed while the writer is in the waiting state
    if (_closed) {
      assert(static_cast<bool>(_hot_potato));
      l = std::move(_hot_potato);
      return std::move(t);
    }

    // write into `_reader`
    assert(_reader != nullptr && static_cast<bool>(_hot_potato) &&
           _waiter != nullptr);
    _reader->emplace(std::move(t));
    _reader = nullptr;
    l = std::move(_hot_potato);
  } else {
    // write called after read
    assert(_reader != nullptr && !_hot_potato);
    l.unlock();
    _reader->emplace(std::move(t));
    _reader = nullptr;
    task* reader = _waiter;
    _waiter = nullptr;
    l.lock();
    _hot_potato = std::move(l);
    s.runqueue_push(task::current());
    reader->switch_to();
  }

  return absl::nullopt;
}
}  // namespace internal
}  // namespace gthread
