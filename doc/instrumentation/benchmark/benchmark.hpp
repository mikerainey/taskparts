#pragma once

// The benchmark harness of TaskPaRTS (benchmark/benchmark.hpp at the top of
// this repository), for micro taskparts. It has the same interface, so the
// driver programs written for that harness compile against this one
// unchanged.
//
// benchmark(body, setup, teardown, reset) calls setup, then body a number of
// times, and measures each call of body as a region of its own, between two
// snapshots. Whatever the program does before the call of benchmark, such as
// generating its input, and what setup and reset do, is outside every
// region. It then writes one record per region, as a JSON list, and calls
// teardown.
//
//   TASKPARTS_BENCHMARK_WARMUP_SECS    call body, unmeasured, for this long
//                                      first (default 0)
//   TASKPARTS_BENCHMARK_NUM_REPEAT     the number of regions (default 1)
//   TASKPARTS_BENCHMARK_STATS_OUTFILE  where the records go (default stdout)
//
// With ParlayLib's sequential build (PARLAY_SEQUENTIAL) or its own scheduler
// (PARLAY_HOMEGROWN) there is no micro taskparts, and a record holds the
// time of the region only.

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#if defined(PARLAY_TASKPARTS)
#include "microtaskparts.hpp"
#endif

namespace taskparts {

#if defined(PARLAY_TASKPARTS)
using benchmark_snapshot = microtaskparts::snapshot;
inline benchmark_snapshot benchmark_now() {
  return microtaskparts::take_snapshot();
}
#else
using benchmark_snapshot = std::chrono::steady_clock::time_point;
inline benchmark_snapshot benchmark_now() {
  return std::chrono::steady_clock::now();
}
#endif

inline void benchmark_report(FILE *f, const benchmark_snapshot &a,
                             const benchmark_snapshot &b) {
#if defined(PARLAY_TASKPARTS)
  microtaskparts::stats s = microtaskparts::stats_between(a, b);
  std::fprintf(f, "{\"exectime\": %.6f", s.exectime);
#if defined(MTP_STATS)
  std::fprintf(f,
               ", \"nb_workers\": %zu, \"work\": %.6f, \"sched\": %.6f, "
               "\"idle\": %.6f, \"nb_steals\": %llu, \"nb_forks\": %llu",
               s.nb_workers, s.work, s.sched, s.idle,
               (unsigned long long)s.nb_steals, (unsigned long long)s.nb_forks);
#endif
  std::fprintf(f, "}");
#else
  std::chrono::duration<double> elapsed = b - a;
  std::fprintf(f, "{\"exectime\": %.6f}", elapsed.count());
#endif
}

inline double benchmark_env(const char *name, double otherwise) {
  const char *s = std::getenv(name);
  return (s != nullptr) ? std::atof(s) : otherwise;
}

template <typename Benchmark, typename Setup, typename Teardown,
          typename Reset>
auto benchmark(const Benchmark &body, const Setup &setup,
               const Teardown &teardown, const Reset &reset) -> void {
  setup();
  double warmup_secs = benchmark_env("TASKPARTS_BENCHMARK_WARMUP_SECS", 0.0);
  if (warmup_secs > 0.0) {
    auto start = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed{0.0};
    while (elapsed.count() < warmup_secs) {
      body();
      reset();
      elapsed = std::chrono::steady_clock::now() - start;
    }
  }
  size_t nb_repeat =
      (size_t)benchmark_env("TASKPARTS_BENCHMARK_NUM_REPEAT", 1.0);
  std::vector<benchmark_snapshot> before, after;
  for (size_t i = 0; i < nb_repeat; i++) {
    before.push_back(benchmark_now());
    body();
    after.push_back(benchmark_now());
    if ((i + 1) < nb_repeat) {
      reset();
    }
  }
  const char *outfile = std::getenv("TASKPARTS_BENCHMARK_STATS_OUTFILE");
  bool to_stdout = (outfile == nullptr) || (std::string(outfile) == "stdout");
  FILE *f = to_stdout ? stdout : std::fopen(outfile, "w");
  if (f == nullptr) {
    std::perror(outfile);
    std::abort();
  }
  std::fprintf(f, "[\n");
  for (size_t i = 0; i < nb_repeat; i++) {
    benchmark_report(f, before[i], after[i]);
    std::fprintf(f, "%s\n", ((i + 1) < nb_repeat) ? "," : "");
  }
  std::fprintf(f, "]\n");
  if (!to_stdout) {
    std::fclose(f);
  }
  teardown();
}

template <typename Benchmark> auto benchmark(const Benchmark &body) -> void {
  benchmark(body, [] {}, [] {}, [] {});
}

} // namespace taskparts
