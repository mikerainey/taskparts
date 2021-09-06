#pragma once

#include "taskparts/nativeforkjoin.hpp"

int64_t fib_par_nativefj(int64_t n) {
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
