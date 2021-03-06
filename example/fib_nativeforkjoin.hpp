#pragma once

#include <taskparts/nativeforkjoin.hpp>
#include "fib_serial.hpp"

int64_t fib_serial_threshold = 15;

template <typename Scheduler>
auto fib_nativeforkjoin(int64_t n, Scheduler sched=Scheduler()) -> int64_t {
  if (n <= fib_serial_threshold) {
    return fib_serial(n);
  } else {
    int64_t r1, r2;
    taskparts::fork2join([&] {
      r1 = fib_nativeforkjoin(n-1, sched);
    }, [&] {
      r2 = fib_nativeforkjoin(n-2, sched);
    }, sched);
    return r1 + r2;
  }
}
