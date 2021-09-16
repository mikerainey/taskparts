#include <stdio.h>
#include <assert.h>

#include "../example/fib_nativeforkjoin.hpp"
#include "taskparts/benchmark.hpp"

int main() {
  int64_t n = 45;
  int64_t dst;
  taskparts_die("todo");
  assert(dst == fib_sequential(n));
  return 0;
}
