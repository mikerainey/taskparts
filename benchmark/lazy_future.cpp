#include <stdio.h>
#include <assert.h>

#include "../example/fib_serial.hpp"
#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/posix/diagnostics.hpp"
#include <taskparts/benchmark.hpp>

int64_t *a;
int64_t m;

using namespace taskparts;

template <typename Scheduler>
auto loop(int lo, int hi, Scheduler sched) -> void {
  if (lo == hi) {
    return;
  }
  if ((lo + 1) == hi) {
    TASKPARTS_LOG_PPT(Scheduler, (void*)lo);
    auto x = m + (hash(lo) % 4);
    a[lo] = fib_serial(x);
    TASKPARTS_LOG_PPT(Scheduler, (void*)lo);
    return;
  }
  assert(lo < hi); 
  auto mid = (lo + hi) / 2;
  auto fut = spawn_lazy_future([&] { loop(mid, hi, sched); }, sched);
  loop(lo, mid, sched);
  fut->force();
}

int main() {
  int64_t n = cmdline::parse_or_default_long("n", 29);
  m = cmdline::parse_or_default_long("m", 10);
  a = new int64_t[n];
  benchmark_nativeforkjoin([&] (auto sched){
    loop(0, n, sched);
  });
  printf("result %lu\n",a[2]);
  delete [] a;
  return 0;
}
