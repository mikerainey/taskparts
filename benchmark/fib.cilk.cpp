#include <stdio.h>
#include <assert.h>

#include "cilk.hpp"
#include "../example/fib_serial.hpp"

int64_t fib_serial_threshold = 15;

auto fib_cilk(int64_t n) -> int64_t {
  if (n <= fib_serial_threshold) {
    return fib_serial(n);
  } else {
    int64_t r1, r2;
    r1 = cilk_spawn fib_cilk(n-1);
    r2 = fib_cilk(n-2);
    cilk_sync;
    return r1 + r2;
  }
}

int main() {
  int64_t n = taskparts::cmdline::parse_or_default_long("n", 44);
  int64_t dst;
  taskparts::benchmark_cilk([&] {
    dst = fib_cilk(n);
  });
  printf("result %lu\n",dst);
  return 0;
}
