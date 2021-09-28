#pragma once

#include <math.h>

#include <taskparts/oracleguided.hpp>
#include "fib_sequential.hpp"

template <typename Scheduler>
auto fib_oracleguided(int64_t n, Scheduler sched=Scheduler()) -> int64_t {
  double phi = (1 + sqrt(5)) / 2;
  int64_t r;
  taskparts::spguard([&] { return pow(phi, n); }, [&] {
    if (n <= 1) {
      r = n;
    } else {
      int64_t r1, r2;
      taskparts::ogfork2join([&] {
	r1 = fib_oracleguided(n-1, sched);
      }, [&] {
	r2 = fib_oracleguided(n-2, sched);
      }, sched);
      r = r1 + r2;
    }
  }, [&] {
    r = fib_sequential(n);
  });
  return r;
}
