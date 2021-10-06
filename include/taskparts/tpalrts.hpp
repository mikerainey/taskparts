#pragma once

#include <cstdint>

#include "machine.hpp"
#include "nativeforkjoin.hpp"

namespace taskparts {

using ping_thread_status_type = enum ping_thread_status_enum {
  ping_thread_status_active,
  ping_thread_status_exit_launch,
  ping_thread_status_exited
};

using promote_status_type = enum promote_status_enum {
  tpal_promote_fail = 0,
  tpal_promote_success = 1
};

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
    return 1;
  }
  fb1.stack = fiber_type::notownstackptr;
  fb1.swap_with_scheduler();
  fb1.run();
  auto f = Scheduler::template take<nativefj_fiber>();
  if (f == nullptr) {
    cfb->status = fiber_status_finish;
    cfb->exit_to_scheduler();
    return 1; // unreachable
  }
  fb2.stack = fiber_type::notownstackptr;
  fb2.swap_with_scheduler();
  fb2.run();
  cfb->status = fiber_status_finish;
  cfb->swap_with_scheduler();
}

} // end namespace

#if defined(TASKPARTS_POSIX)
#include "posix/tpalrts.hpp"
#elif defined (TASKPARTS_NAUTILUS)
#include "nautilus/tpalrts.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_POSIX)
#endif

