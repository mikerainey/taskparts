#pragma once

#include "taskparts/fiber.hpp"

#include <cstdint>

auto fib_seq(int64_t n) -> int64_t {
  if (n <= 1) {
    return n;
  } else {
    return fib_seq(n-1) + fib_seq(n-2);
  }
}

int64_t fib_T = 15;

template <typename Scheduler>
class fib_fiber : public taskparts::fiber<Scheduler> {
public:

  using trampoline_type = enum { entry, exit };

  trampoline_type trampoline = entry;

  int64_t n; int64_t* dst;
  int64_t d1, d2;

  fib_fiber(int64_t n, int64_t* dst)
    : taskparts::fiber<Scheduler>(), n(n), dst(dst) { }

  auto run() -> taskparts::fiber_status_type {
    switch (trampoline) {
    case entry: {
      if (n <= fib_T) {
        *dst = fib_seq(n);
        break;
      }
      auto f1 = new fib_fiber(n-1, &d1);
      auto f2 = new fib_fiber(n-2, &d2);
      taskparts::fiber<Scheduler>::add_edge(f1, this);
      taskparts::fiber<Scheduler>::add_edge(f2, this);
      f1->release();
      f2->release();
      trampoline = exit;
      return taskparts::fiber_status_pause;	  
    }
    case exit: {
      *dst = d1 + d2;
      break;
    }
    }
    return taskparts::fiber_status_finish;
  }
  
};
