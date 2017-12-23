#include "sched/task.h"

#include "benchmark/benchmark.h"

static void* mirror(void* arg) {
  auto* t = (gthread::task*)arg;
  while (true) t->switch_to();
  return NULL;
}

static void benchmark_task_switch_to_no_tls(benchmark::State& state) {
  auto root_task = gthread::task::create_wrapped();
  auto* t = gthread::task::create(gthread::k_light_attr);
  t->entry = mirror;
  t->arg = &root_task;
  t->start();

  for (auto _ : state) {
    t->switch_to();
  }

  t->destroy();
}

BENCHMARK(benchmark_task_switch_to_no_tls);

static void benchmark_task_switch_to_with_tls(benchmark::State& state) {
  auto root_task = gthread::task::create_wrapped();
  auto* t = gthread::task::create(gthread::k_default_attr);
  t->entry = mirror;
  t->arg = &root_task;
  t->start();

  for (auto _ : state) {
    t->switch_to();
  }

  t->destroy();
}

BENCHMARK(benchmark_task_switch_to_with_tls);

static void benchmark_task_start(benchmark::State& state) {
  auto* t = gthread::task::create(gthread::k_default_attr);

  for (auto _ : state) {
    t->start();
    t->run_state = gthread::task::STOPPED;
  }

  t->destroy();
}

BENCHMARK(benchmark_task_start);

static void benchmark_task_create_destroy_no_tls(benchmark::State& state) {
  for (auto _ : state) {
    auto* t = gthread::task::create(gthread::k_light_attr);
    t->destroy();
  }
}

BENCHMARK(benchmark_task_create_destroy_no_tls);

static void benchmark_task_create_destroy_with_tls(benchmark::State& state) {
  for (auto _ : state) {
    auto* t = gthread::task::create(gthread::k_default_attr);
    t->destroy();
  }
}

BENCHMARK(benchmark_task_create_destroy_with_tls);

static void benchmark_task_reset_no_tls(benchmark::State& state) {
  auto* t = gthread::task::create(gthread::k_light_attr);

  for (auto _ : state) {
    t->reset();
  }

  t->destroy();
}

BENCHMARK(benchmark_task_reset_no_tls);

static void benchmark_task_reset_with_tls(benchmark::State& state) {
  auto* t = gthread::task::create(gthread::k_default_attr);

  for (auto _ : state) {
    t->reset();
  }

  t->destroy();
}

BENCHMARK(benchmark_task_reset_with_tls);

BENCHMARK_MAIN();
