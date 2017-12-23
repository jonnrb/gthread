#pragma once

#include <iostream>
#include <sstream>

template <typename Stream, typename T>
auto& gthread_recursive_write(Stream& stream, T&& t) {
  return stream << std::forward<T>(t);
}

template <typename Stream, typename T, typename... Rest>
auto& gthread_recursive_write(Stream& stream, T&& t, Rest&&... rest) {
  return gthread_recursive_write(stream << std::forward<T>(t),
                                 std::forward<Rest>(rest)...);
}

template <typename... Args>
void gthread_log(Args&&... args) {
  gthread_recursive_write(std::cerr, std::forward<Args>(args)...) << std::endl;
}

template <typename... Args>
void gthread_log_fatal(Args&&... args) {
  gthread_recursive_write(std::cerr, std::forward<Args>(args)...) << std::endl;
  std::stringstream ss;
  gthread_recursive_write(ss, std::forward<Args>(args)...);
  throw std::runtime_error(ss.str());
}
