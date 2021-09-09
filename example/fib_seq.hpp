#pragma once

#include <cstdint>

auto fib_seq(int64_t n) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    return fib_seq(n-1) + fib_seq(n-2);
  }
}
