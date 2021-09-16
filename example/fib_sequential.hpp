#pragma once

#include <cstdint>

auto fib_sequential(int64_t n) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    return fib_sequential(n-1) + fib_sequential(n-2);
  }
}
