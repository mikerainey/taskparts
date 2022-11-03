#include <taskparts/defaults.hpp>
#include <taskparts/workstealing.hpp>
#include "taskparts/cmdline.hpp"
#include <fib_nativeforkjoin.hpp>

/*
clang++ -std=c++17 -g3 -O0 -I. -I../include -DTASKPARTS_DARWIN -DTASKPARTS_ARM64 -DTASKPARTS_STATS fib_nativeforkjoin.cpp
TASKPARTS_CPU_BASE_FREQUENCY_KHZ=3200000 ./a.out
 */


int main() {
  auto n = taskparts::cmdline::parse_or_default_int("n", 45);
  int64_t r = 0;
  taskparts::launch([&] (auto sched) {
    r = fib_nativeforkjoin(n, sched);
  });
  printf("r\t%ld\n", r);
  return 0;
}
