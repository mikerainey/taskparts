#pragma once

#if defined(PARLAY_SEQUENTIAL)
#include <functional>
#include <cstdio>
#include <chrono>
#include <string>
#include <vector>

static inline
auto now() -> std::chrono::time_point<std::chrono::steady_clock> {
  return std::chrono::steady_clock::now();
}

static inline
auto diff(std::chrono::time_point<std::chrono::steady_clock> start,
          std::chrono::time_point<std::chrono::steady_clock> finish) -> double {
  std::chrono::duration<double> elapsed = finish - start;
  return elapsed.count();
}

static inline
auto since(std::chrono::time_point<std::chrono::steady_clock> start) -> double {
  return diff(start, now());
}

auto get_benchmark_warmup_secs() -> double {
  return 0.0;
}

auto get_benchmark_verbose() -> bool {
  return false;
}

auto get_benchmark_nb_repeat() -> size_t {
  size_t nb = 1;
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_NUM_REPEAT")) {
    nb = std::stol(env_p);
  }
  return nb;
}

std::chrono::time_point<std::chrono::steady_clock> start_time;
std::vector<double> wall_clock_times;

auto instrumentation_start() -> void {

}
auto instrumentation_on_enter_work() -> void {
  start_time = now();  
}
auto instrumentation_on_exit_work() -> void {
  wall_clock_times.push_back(since(start_time));
  start_time = now();  
}
auto instrumentation_reset() -> void {
}
auto instrumentation_capture() -> void {}
auto instrumentation_report(std::string outfile) -> void {
  size_t i = 0;
  FILE* f = (outfile == "stdout") ? stdout : fopen(outfile.c_str(), "w");
  fprintf(f, "[\n");
  for (auto& t : wall_clock_times) {
    fprintf(f, "{\"exectime\": %f}%s\n", t, ((i + 1) == wall_clock_times.size()) ? "" : ",");
    i++;
  }
  fprintf(f, "]\n");
}
template <typename Local_reset, typename Global_reset>
auto reset_scheduler(const Local_reset& local_reset,
                     const Global_reset& global_reset,
                     bool global_first) -> void {
  if (global_first) {
    global_reset();
    local_reset();
  } else {
    local_reset();
    global_reset();
  }
}

#elif defined(PARLAY_TASKPARTS)
#include <taskparts/taskparts.hpp>
#else
#error "need to provide parlay scheduler option"
#endif

#include <parlay/primitives.h>

namespace taskparts {

template <
typename Benchmark,
typename Setup = std::function<void()>,
typename Teardown = std::function<void()>,
typename Reset = std::function<void()>>
auto benchmark(const Benchmark& benchmark,
               const Setup& setup = [] {},
               const Teardown& teardown = [] {},
               const Reset& reset = [] {}) -> void {
  auto warmup = [&] {
    if (get_benchmark_warmup_secs() <= 0.0) {
      return;
    }
    if (get_benchmark_verbose()) printf("======== WARMUP ========\n");
    auto warmup_start = now();
    while (since(warmup_start) < get_benchmark_warmup_secs()) {
      auto st = now();
      benchmark();
      if (get_benchmark_verbose()) printf("warmup_run %.3f\n", since(st));
      reset();
    }
    if (get_benchmark_verbose()) printf ("======== END WARMUP ========\n");
  };
  setup();
  warmup();
  for (size_t i = 0; i < get_benchmark_nb_repeat(); i++) {
    reset_scheduler([&] { // worker local
      instrumentation_on_enter_work();
    }, [&] { // global
      instrumentation_reset();
      instrumentation_start();
    }, false);
    benchmark();
    reset_scheduler([&] { // worker local
      instrumentation_on_exit_work();
    }, [&] { // global
      instrumentation_capture();
      if ((i + 1) < get_benchmark_nb_repeat()) {
        reset();
      }
      instrumentation_start();
    }, true);
  }
  std::string outfile = "stdout";
  if (const auto env_p = std::getenv("TASKPARTS_BENCHMARK_STATS_OUTFILE")) {
    outfile = std::string(env_p);
  }
  instrumentation_report(outfile);
  teardown();
}

}
