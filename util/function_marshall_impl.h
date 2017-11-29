#pragma once

#include "util/function_marshall.h"

#include <functional>
#include <tuple>

#include "util/compiler.h"

namespace gthread {
template <typename Function, typename... Args>
function_marshall<Function, Args...>::function_marshall(Function&& function,
                                                        Args&&... args)
    : _used(false),
      _function(std::forward<Function>(function)),
      _args(std::forward<Args>(args)...) {}

template <typename Function, typename... Args>
typename std::result_of<Function(Args...)>::type
function_marshall<Function, Args...>::operator()() {
  if (branch_unexpected(_used)) throw std::bad_function_call();
  _used = true;
  return std::apply(_function, std::move(_args));
}
}  // namespace gthread
