#pragma once

#include <iostream>

static inline void gthread_log(std::string message) {
  std::cerr << message << std::endl;
}

static inline void gthread_log_fatal(std::string message_of_death) {
  std::cerr << message_of_death << std::endl;
  throw std::runtime_error(message_of_death);
}
