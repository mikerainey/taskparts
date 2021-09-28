#pragma once

#include <taskparts/fiber.hpp>
#include "fib_sequential.hpp"

int64_t fib_sequential_threshold = 15;

template <typename Scheduler=taskparts::minimal_scheduler<>>
class fib_manualfiber : public taskparts::fiber<Scheduler> {
public:

  using trampoline_type = enum trampoline_enum { entry, exit };

  trampoline_type trampoline = entry;

  int64_t n; int64_t* dst;
  int64_t d1, d2;

  fib_manualfiber(int64_t n, int64_t* dst, Scheduler sched=Scheduler())
    : taskparts::fiber<Scheduler>(), n(n), dst(dst) { }

  auto run() -> taskparts::fiber_status_type {
    switch (trampoline) {
    case entry: {
      if (n <= fib_sequential_threshold) {
        *dst = fib_sequential(n);
        break;
      }
      auto f1 = new fib_manualfiber(n-1, &d1);
      auto f2 = new fib_manualfiber(n-2, &d2);
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
