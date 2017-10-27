#pragma once

#include <functional>

namespace gthread {
/**
 * simple RAII timer (move only)
 *
 * runs a callback on release of the timer with the amount of elapsed time
 * while the timer was held
 */
template <typename clock>
class timer {
 public:
  using end_function = std::function<void(typename clock::duration)>;

  timer(const end_function& f) : _start(clock::now()), _end_function(f) {}

  timer(const timer&) = delete;

  timer(timer&& t) : _start{t._start}, _end_function{nullptr} {
    std::swap(_end_function, t._end_function);
  }

  ~timer() {
    if (_end_function) {
      _end_function(clock::now() - _start);
    }
  }

 private:
  typename clock::time_point _start;
  end_function _end_function;
};
}  // namespace gthread
