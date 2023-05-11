#pragma once

#include <cstdint>

auto fib_serial(int64_t n) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    return fib_serial(n-1) + fib_serial(n-2);
  }
}
