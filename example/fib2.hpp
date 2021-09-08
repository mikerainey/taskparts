#pragma once

#include "taskparts/nativeforkjoin.hpp"

auto fib_par_nativefj(int64_t n) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    int64_t r1, r2;
    taskparts::fork2([&] {
      r1 = fib_par_nativefj(n-1);
    }, [&] {
      r2 = fib_par_nativefj(n-2);
    });
    return r1 + r2;
  }
}
