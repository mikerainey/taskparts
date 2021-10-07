#pragma once

#include <cstdint>

#include "defaults.hpp"

namespace taskparts {

template <typename F1, typename F2, typename Fj, typename Scheduler>
auto tpalrts_promote_via_nativefj(const F1& f1, const F2& f2, const Fj& fj, Scheduler sched) {
  using fiber_type = nativefj_fiber<Scheduler>;
  nativefj_from_lambda<decltype(f1), Scheduler> fb1(f1);
  nativefj_from_lambda<decltype(f2), Scheduler> fb2(f2);
  nativefj_from_lambda<decltype(fj), Scheduler> fbj(fj);
  auto cfb = fiber_type::current_fiber.mine();
  cfb->status = fiber_status_pause;
  fiber<Scheduler>::add_edge(&fb1, &fbj);
  fiber<Scheduler>::add_edge(&fb2, &fbj);
  fiber<Scheduler>::add_edge(&fbj, cfb);
  fb1.release();
  fb2.release();
  fbj.release();
  if (context::capture<fiber_type*>(context::addr(cfb->ctx))) {
    return;
  }
  fb1.stack = fiber_type::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = Scheduler::template take<nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = fiber_status_finish;
    cfb->exit_to_scheduler();
    return; // unreachable
  }
  fb2.stack = fiber_type::notownstackptr;
  fb2.swap_with_scheduler();
  fb2.run();
  cfb->status = fiber_status_finish;
  cfb->swap_with_scheduler();
}
  
/* Interface for TPAL runtime 
 *   - assumptions: leaf loop (non nested), no reduction
 *   - return nonzero value if promotion happened, zero otherwise
 *   - indvar: loop induction variable, maxval: loop maximum value
 *   - ...
 *   int handler_for_fork2(int64_t indvar, int64_t maxval, 
 *                         void** livein, void(*f)(int64_t,int64_t,void**)) {
 *     ... here, we call a C++ function in the TPAL runtime
 *   }
 */

int tpalrts_promote(int64_t indvar, int64_t maxval,
		    void** livein,
		    void(*f)(int64_t,int64_t,void**)) {
  auto n = maxval - indvar;
  if (n == 0) {
    return 0;
  }
  int64_t mid = indvar + (n / 2);
  tpalrts_promote_via_nativefj([&] {
    f(indvar, mid, livein);
  }, [&] {
    f(mid, n, livein);
  }, [&] { }, bench_scheduler());
  return 1;
}
  
} // end namespace


