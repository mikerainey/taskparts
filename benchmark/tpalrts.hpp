#pragma once

#include "taskparts/chaselev.hpp"
#include "taskparts/tpalrts.hpp"
#include "taskparts/nativeforkjoin.hpp"

namespace taskparts {
  
using tpalrts = minimal_scheduler<minimal_stats, minimal_logging, minimal_elastic,
				  ping_thread_worker, ping_thread_interrupt>;
using tpalrts_chaselev = chase_lev_work_stealing_scheduler<tpalrts, fiber,
						       minimal_stats, minimal_logging, minimal_elastic,
						       ping_thread_worker, ping_thread_interrupt>;

template <typename F>
auto launchtpalrts(const F& f) {
  initialize_machine();
  auto f0 = [&] {
    f();
  };
  taskparts::nativefj_from_lambda<decltype(f0), taskparts::tpalrts> f_body(f0);
  auto f_term = new taskparts::terminal_fiber<taskparts::tpalrts>;
  taskparts::fiber<taskparts::tpalrts>::add_edge(&f_body, f_term);
  f_body.release();
  f_term->release();
  tpalrts_chaselev::launch();
  teardown_machine();
}
  
} // end namespace
