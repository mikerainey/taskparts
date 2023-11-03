#pragma once

#if defined(PARLAY_SEQUENTIAL) || defined(PARLAY_HOMEGROWN) || defined(PARLAY_OPENCILK)
#include <functional>
#include <cstdio>
#include <chrono>
#include <string>
#include <vector>
#include <sys/resource.h>

static struct rusage last_rusage;
std::chrono::time_point<std::chrono::steady_clock> last_exectime;

using rusage_metrics = struct rusage_metrics_struct {
  double exectime;
  double utime;
  double stime;
  uint64_t nvcsw;
  uint64_t nivcsw;
  uint64_t maxrss;
  uint64_t nsignals;
};
std::vector<rusage_metrics> rusages;

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

auto instrumentation_start() -> void {

}
auto instrumentation_on_enter_work() -> void {
  getrusage(RUSAGE_SELF, &last_rusage);
  last_exectime = now();
}
auto instrumentation_on_exit_work() -> void {
  {
    auto double_of_tv = [] (struct timeval tv) {
      return ((double) tv.tv_sec) + ((double) tv.tv_usec)/1000000.;
    };
    auto exectime = since(last_exectime);
    last_exectime = now();
    auto previous_rusage = last_rusage;
    getrusage(RUSAGE_SELF, &last_rusage);
    rusage_metrics m = {
      .exectime = exectime,
      .utime = double_of_tv(last_rusage.ru_utime) - double_of_tv(previous_rusage.ru_utime),
      .stime = double_of_tv(last_rusage.ru_stime) - double_of_tv(previous_rusage.ru_stime),
      .nvcsw = (uint64_t)(last_rusage.ru_nvcsw - previous_rusage.ru_nvcsw),
      .nivcsw = (uint64_t)(last_rusage.ru_nivcsw - previous_rusage.ru_nivcsw),
      .maxrss = (uint64_t)(last_rusage.ru_maxrss),
      .nsignals = (uint64_t)(last_rusage.ru_nsignals - previous_rusage.ru_nsignals)
    };
    rusages.push_back(m);
  }
}
auto instrumentation_reset() -> void {
}
auto instrumentation_capture() -> void {}
auto instrumentation_report(std::string outfile) -> void {
  size_t i = 0;
  FILE* f = (outfile == "stdout") ? stdout : fopen(outfile.c_str(), "w");
  fprintf(f, "[\n");
  size_t n = rusages.size();
  for (size_t i = 0; i < n; i++) {
    fprintf(f, "{\"exectime\": %f,\n", rusages[i].exectime);
    fprintf(f, "\"usertime\": %f,\n", rusages[i].utime);
    fprintf(f, "\"systime\": %f,\n", rusages[i].stime);
    fprintf(f, "\"nvcsw\": %lu,\n", rusages[i].nvcsw);
    fprintf(f, "\"nivcsw\": %lu,\n", rusages[i].nivcsw);
    fprintf(f, "\"maxrss\": %lu,\n", rusages[i].maxrss);
    fprintf(f, "\"nsignals\": %lu}", rusages[i].nsignals);
    if (i + 1 != n) {
      fprintf(f, ",\n");
    }
  }
  fprintf(f, "\n]\n");
  if (f != stdout) {
    fclose(f);
  }
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
