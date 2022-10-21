#include <cstdint>
#include <stdio.h>
#include <assert.h>

#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/perworker.hpp"
#include "taskparts/timing.hpp"
#include <taskparts/benchmark.hpp>

using namespace taskparts;

int D = 64;

int64_t H = 100000;

perworker::array<uint64_t> prev;

template <typename Scheduler>
auto sum_array(double* a, int64_t lo, int64_t hi, double* dst, Scheduler sched) -> void;

template <typename Scheduler>
auto sum_array_handler(double* a, int64_t lo, int64_t hi, double* dst,
		       future*& f, Scheduler sched) {
  assert(f == nullptr);
  if ((hi - lo) < 3) {
    return;
  }
  auto& p = prev.mine();
  auto n = cycles::now();
  if ((n - p) < H) {
    return;
  }
  p = n;
  lo++;
  f = spawn_lazy_future([=] {
    double dst1, dst2;
    auto mid = (lo + hi) / 2;
    auto f = spawn_lazy_future([&] {
      sum_array(a, mid, hi, &dst2, sched);
    }, sched);
    sum_array(a, lo, mid, &dst1, sched);
    f->force();
    *dst += dst1 + dst2;
  }, sched);
}

int64_t n;

template <typename Scheduler>
auto sum_array(double* a, int64_t lo, int64_t hi, double* dst, Scheduler sched) -> void {
  double r = 0.0;
  future* f = nullptr;
  while (lo < hi) {
    sum_array_handler(a, lo, hi, dst, f, sched);
    hi = (f == nullptr) ? hi : std::min(lo + 1, hi);
    auto lo2 = lo;
    auto hi2 = std::min(lo2 + D, hi);
    double r2 = r;
    for (; lo2 < hi2; lo2++) {
      r2 += a[lo2];
    }
    lo = lo2;
    r = r2;
  }
  if (f != nullptr) {
    f->force();
  }
  *dst += r;
}

double* a0 = nullptr;

int main() {
  n = taskparts::cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  D = taskparts::cmdline::parse_or_default_int("D", D);
  H = taskparts::cmdline::parse_or_default_int("H", H);
  double result = 0.0;
  auto init_array = [&] {
    for (int64_t i = 0; i < n; i++) {
      a0[i] = 1.0;
    }
  };
  
  a0 = (double*)malloc(n * sizeof(double));

  for (size_t i = 0; i < prev.size(); i++) {
    prev[i] = cycles::now();
  }
  
  taskparts::benchmark_nativeforkjoin([&] (auto sched){
    sum_array(a0, 0, n, &result, sched);
  }, [&] (auto sched) { // setup

    init_array();
  }, [&] (auto sched) { // teardown
    
  }, [&] (auto sched) { // reset
    init_array();
  });
  
  free(a0);
  a0 = nullptr;

  printf("result\t%.0f\n", result);
  return 0;
}
