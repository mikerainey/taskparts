#pragma once

#include <math.h>

#include "taskparts/nativeforkjoin.hpp"
#include "taskparts/oracleguided.hpp"

#include "fib_seq.hpp"

auto fib_oracleguided(int64_t n) -> int64_t {
  double phi = (1 + sqrt(5)) / 2;
  int64_t r;
  taskparts::spguard([&] { return pow(phi, n); }, [&] {
    if (n <= 1) {
      r = n;
    } else {
      int64_t r1, r2;
      taskparts::ogfork2([&] {
	r1 = fib_oracleguided(n-1);
      }, [&] {
	r2 = fib_oracleguided(n-2);
      });
      r = r1 + r2;
    }
  }, [&] {
    r = fib_seq(n);
  });
  return r;
}
