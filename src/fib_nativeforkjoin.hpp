#pragma once

#include <taskparts/taskparts.hpp>
#include "fib_serial.hpp"

int64_t fib_serial_threshold = 15;

auto fib_nativeforkjoin(int64_t n) -> int64_t {
  if (n <= fib_serial_threshold) {
    return fib_serial(n);
  } else {
    int64_t r1, r2;
    taskparts::fork2join([&] {
      r1 = fib_nativeforkjoin(n-1);
    }, [&] {
      r2 = fib_nativeforkjoin(n-2);
    });
    return r1 + r2;
  }
}
