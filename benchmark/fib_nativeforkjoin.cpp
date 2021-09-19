#include <stdio.h>
#include <assert.h>

#include "../example/fib_nativeforkjoin.hpp"
#include "taskparts/benchmark.hpp"

int main() {
  int64_t n = 45;
  int64_t dst;
  taskparts::benchmark_nativeforkjoin([&] (auto sched){
    dst = fib_nativeforkjoin(n, sched);
    printf("result %lu\n",dst);
  });
  assert(dst == fib_sequential(n));
  return 0;
}