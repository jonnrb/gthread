#include <iostream>
#include <thread>

constexpr uint64_t k_num_threads = 1000;

void sleepy_test_thread() {
  std::this_thread::sleep_for(std::chrono::milliseconds{50});
}

int main() {
  std::thread threads[k_num_threads];

  auto start = std::chrono::system_clock::now();
  for (int i = 0; i < 3; ++i) {
    std::cout << "spawning " << k_num_threads << " sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      threads[i] = std::thread(sleepy_test_thread);
    }

    std::cout << "joining ALL the sleepy threads" << std::endl;
    for (uint64_t i = 0; i < k_num_threads; ++i) {
      threads[i].join();
    }
  }
  std::cout << "std::thread took "
            << std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::system_clock::now() - start)
                   .count()
            << " µs" << std::endl;

  return 0;
}
