#pragma once

// What the benchmark programs of this directory share: the measured region,
// the lines they print, and a parallel loop made of binary forks.

#include "../microtaskparts/microtaskparts.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace bench {

using namespace microtaskparts;

// The region that a program measures. A version that uses the scheduler
// takes a snapshot on each side, and the first starts the worker threads,
// outside the region. A serial version starts no thread.
struct region {
  bool parallel = false;
  double exectime = 0.0;
  stats st;
};

template <typename Body> region measure(bool parallel, const Body &body) {
  region r;
  r.parallel = parallel;
  snapshot before;
  if (parallel) {
    before = take_snapshot();
  }
  auto start = std::chrono::steady_clock::now();
  if (parallel) {
    launch(body);
  } else {
    body();
  }
  std::chrono::duration<double> elapsed =
      std::chrono::steady_clock::now() - start;
  r.exectime = elapsed.count();
  if (parallel) {
    r.st = stats_between(before, take_snapshot());
    r.exectime = r.st.exectime;
  }
  return r;
}

inline void report(const region &r) {
  std::printf("workers   %zu\n", get_nb_workers());
  std::printf("exectime  %.3f s\n", r.exectime);
  if (r.parallel) {
    print_stats(stdout, r.st);
  }
}

inline int report_check(bool same) {
  std::printf("check     %s\n", same ? "ok" : "WRONG");
  return same ? 0 : 1;
}

inline bool is(const char *a, const char *b) { return std::strcmp(a, b) == 0; }

// Whether argument k is the word "check".
inline bool wants_check(int argc, char **argv, int k) {
  return (argc > k) && is(argv[k], "check");
}

inline long arg(int argc, char **argv, int k, long otherwise) {
  return (argc > k) ? std::atol(argv[k]) : otherwise;
}

// f(i) for lo <= i < hi, in parallel.
template <typename F> void parallel_for(long lo, long hi, const F &f) {
  if (hi - lo <= 0) {
    return;
  }
  if (hi - lo == 1) {
    f(lo);
    return;
  }
  long mid = lo + (hi - lo) / 2;
  fork2join([&] { parallel_for(lo, mid, f); },
            [&] { parallel_for(mid, hi, f); });
}

} // namespace bench
