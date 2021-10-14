#include <stdio.h>
#include <taskparts/benchmark.hpp>

#include "../example/fib_serial.hpp"

int main() {
  int64_t n = taskparts::cmdline::parse_or_default_long("n", 44);
  int64_t dst;
  taskparts::benchmark_nativeforkjoin([&] (auto sched) {
    dst = fib_serial(n);
  });
  printf("result %lu\n",dst);
  return 0;
}
