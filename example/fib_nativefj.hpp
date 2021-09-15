#pragma once

#include "taskparts/nativeforkjoin.hpp"

template <typename Scheduler>
auto fib_nativefj(int64_t n, Scheduler sched=Scheduler()) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    int64_t r1, r2;
    taskparts::fork2join([&] {
      r1 = fib_nativefj(n-1, sched);
    }, [&] {
      r2 = fib_nativefj(n-2, sched);
    }, sched);
    return r1 + r2;
  }
}
