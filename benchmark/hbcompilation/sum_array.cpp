#include <cstdint>
#include <stdio.h>
#include <assert.h>

#include <taskparts/nativeforkjoin.hpp>
#include <taskparts/benchmark.hpp>

using namespace taskparts;

int D = 64;

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
  if ((p + kappa_cycles) > n) {
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

int main() {
  // TASKPARTS_KAPPA_USEC
  int64_t n = cmdline::parse_or_default_long("n", 1000 * 1000 * 1000);
  D = cmdline::parse_or_default_int("D", D);
  double* a0 = nullptr;
  double result = 0.0;
  auto init_array = [&] {
    for (int64_t i = 0; i < n; i++) {
      a0[i] = 1.0;
    }
  };

  benchmark_nativeforkjoin([&] (auto sched){
    sum_array(a0, 0, n, &result, sched);
  }, [&] (auto sched) { // setup
    a0 = (double*)malloc(n * sizeof(double));
    init_array();
  }, [&] (auto sched) { // teardown
    free(a0);
    a0 = nullptr;
  }, [&] (auto sched) { // reset
    result = 0.0;
    init_array();
  });
  
  printf("result\t%.0f\n", result);
  return 0;
}
