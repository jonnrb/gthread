#include "concur/channel.h"

#include <benchmark/benchmark.h>

#include "sched/preempt.h"

static void bench_channel_sending_ints(benchmark::State& state) {
  auto c = gthread::make_channel<int>();

  gthread::disable_timer_preemption();

  gthread::g reader([read = std::move(c.read)]() {
    while (auto val = read())
      ;
  });

  unsigned i = 0;
  for (auto _ : state) {
    c.write(i++);
  }

  { auto _ = std::move(c.write); }
  reader.join();
  gthread::disable_timer_preemption();
}

BENCHMARK(bench_channel_sending_ints);

static void bench_buffered_channel_sending_ints_64(benchmark::State& state) {
  auto c = gthread::make_buffered_channel<int, 64>();

  gthread::disable_timer_preemption();

  gthread::g reader([read = std::move(c.read)]() {
    while (auto val = read())
      ;
  });

  unsigned i = 0;
  for (auto _ : state) {
    c.write(i++);
  }

  { auto _ = std::move(c.write); }
  reader.join();
  gthread::disable_timer_preemption();
}

BENCHMARK(bench_buffered_channel_sending_ints_64);

static void bench_buffered_channel_sending_ints_1024(benchmark::State& state) {
  auto c = gthread::make_buffered_channel<int, 1024>();

  gthread::disable_timer_preemption();

  gthread::g reader([read = std::move(c.read)]() {
    while (auto val = read())
      ;
  });

  unsigned i = 0;
  for (auto _ : state) {
    c.write(i++);
  }

  { auto _ = std::move(c.write); }
  reader.join();
  gthread::disable_timer_preemption();
}

BENCHMARK(bench_buffered_channel_sending_ints_1024);

static void bench_buffered_channel_sending_ints_4096(benchmark::State& state) {
  auto c = gthread::make_buffered_channel<int, 4096>();

  gthread::disable_timer_preemption();

  gthread::g reader([read = std::move(c.read)]() {
    while (auto val = read())
      ;
  });

  unsigned i = 0;
  for (auto _ : state) {
    c.write(i++);
  }

  { auto _ = std::move(c.write); }
  reader.join();
  gthread::disable_timer_preemption();
}

BENCHMARK(bench_buffered_channel_sending_ints_4096);

BENCHMARK_MAIN()
