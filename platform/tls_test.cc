#include "platform/tls.h"

#include <cassert>
#include <chrono>
#include <iostream>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include "platform/clock.h"
#include "platform/memory.h"

using namespace gthread;

tls* alloc_tls() {
  size_t prefix = tls::prefix_bytes();
  char* storage = new char[prefix + sizeof(tls) + tls::postfix_bytes()];
  return new (storage + prefix) tls();
}

void free_tls(tls* t) {
  size_t prefix = tls::prefix_bytes();
  delete[]((char*)t - prefix);
}

constexpr int baseval = 15;
thread_local int x = baseval;

void x_inc(tls* t, int asserted_value) {
  if (t != nullptr) t->use();
  std::cout << "&x: " << &x << std::endl;
  ++x;
  std::cout << "x: " << (x - baseval) << std::endl;
  assert(x - baseval == asserted_value);
}

thread_local struct {
  int nums[10];
  const char* str;
} y = {{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}, "initial val"};

void y_inc(tls* t, int start_val, const char* str) {
  if (t != nullptr) t->use();
  for (int i = 0; i < 10; ++i) {
    std::cout << y.nums[i] << " ";
    assert(y.nums[i] == (i + start_val) % 10);
    y.nums[i] = (y.nums[i] + 1) % 10;
  }
  if (start_val == 0) y.str = str;
  std::cout << y.str << std::endl;
  assert(strcmp(str, y.str) == 0);
}

int main() {
  std::cout << tls::current_thread() << std::endl;
  tls::current()->set_thread((void*)0xbacabacaL);

  int fd = open("./abc", O_WRONLY | O_CREAT, S_IRWXU | S_IRWXG | S_IRWXO);
  if (fd < 0) {
    perror("");
    return -1;
  }

  tls* old = tls::current();
  assert(old != NULL);

  if (write(fd, "abc", 3) < 0) {
    perror("");
    return -1;
  }

  tls* my_tls = alloc_tls();

  void* tcb = (void*)0xDEADBEEFL;
  my_tls->set_thread(tcb);

  assert(my_tls->get_thread() == tcb);
  assert(tls::current_thread() != tcb);
  my_tls->use();
  assert(tls::current_thread() == tcb);

  tls* other = alloc_tls();
  void* other_tcb = (void*)0xf00dea7e5L;
  other->set_thread(other_tcb);
  other->use();
  assert(tls::current_thread() == other_tcb);

  if (write(fd, "def", 4) < 0) {
    perror("");
    return -1;
  }

  std::cout << "testing XXXXXXXXXXXXXXX" << std::endl;
  std::cout << "original" << std::endl;
  x_inc(old, 1);

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  std::cout << "mine :)" << std::endl;
  x_inc(my_tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  fd = open("./abc", O_RDONLY);
  if (fd < 0) {
    perror("");
    return -1;
  }

  std::cout << "original" << std::endl;
  x_inc(old, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  char buf[7];
  if (read(fd, buf, 7) < 0) {
    perror("");
    return -1;
  }
  std::cout << "some file stuff that was also happening btw: " << buf
            << std::endl;

  std::cout << "testing YYYYYYYYYYYYYYY" << std::endl;
  std::cout << "original" << std::endl;
  y_inc(NULL, 0, "og thread");

  if (close(fd) < 0) {
    perror("");
    return -1;
  }

  std::cout << "mine :)" << std::endl;
  y_inc(my_tls, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");

  if (unlink("./abc") < 0) {
    perror("");
    return -1;
  }

  std::cout << "original" << std::endl;
  y_inc(old, 1, "og thread");
  y_inc(NULL, 2, "og thread");
  y_inc(NULL, 3, "og thread");
  y_inc(NULL, 4, "og thread");

  std::cout << "poisoning main tls" << std::endl;
  x = 172;
  y.nums[0] = -9;
  y.nums[5] = -3;

  std::cout << "resetting mine" << std::endl;
  my_tls->reset();

  x_inc(my_tls, 1);
  x_inc(NULL, 2);
  x_inc(NULL, 3);
  x_inc(NULL, 4);

  y_inc(NULL, 0, "some other thread");
  y_inc(NULL, 1, "some other thread");
  y_inc(NULL, 2, "some other thread");
  y_inc(NULL, 3, "some other thread");
  old->use();

  constexpr uint64_t k_num_tls_switches = 1000 * 1000;
  auto start = thread_clock::now();
  for (uint64_t i = 0; i < k_num_tls_switches; ++i) {
    my_tls->use();
    __asm__ __volatile__("" ::: "memory");
    old->use();
    __asm__ __volatile__("" ::: "memory");
  }
  auto end = thread_clock::now();
  auto elapsed =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);

  std::cout << "  " << (k_num_tls_switches * 2) << " tls switches in "
            << elapsed.count() << " microseconds" << std::endl;

  std::cout << "  " << k_num_tls_switches * 2 / ((double)elapsed.count())
            << " tls switches / microsecond" << std::endl;

  free_tls(my_tls);
  free_tls(other);

  return 0;
}
