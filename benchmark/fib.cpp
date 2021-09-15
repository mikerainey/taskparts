#include <map>
#include <stdio.h>
#include <assert.h>

#include "../example/fib_par.hpp"
#include "../example/fib_nativefj.hpp"
#include "../example/fib_oracleguided.hpp"
#include "benchmark.hpp"

namespace taskparts {

auto bench_fib() {
  int64_t n = 45;
  int64_t dst;
  run_benchmark([&] (auto sched){
    dst = fib_oracleguided(n, sched);
    printf("result %lu\n",dst);
  });
}

} // end namespace

int main() {
  taskparts::bench_fib();
  return 0;
}
