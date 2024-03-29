#pragma once

#include <cstdint>

#include "defaults.hpp"
#include "timing.hpp"

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Promotion */

#if 0

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
  fb2.release();
  fb1.release();
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

#else

template <typename F1, typename F2, typename Fj, typename Scheduler>
auto tpalrts_promote_via_nativefj(const F1& f1, const F2& f2, const Fj& fj, Scheduler sched) {
  using fiber_type = nativefj_fiber<Scheduler>;
  nativefj_from_lambda<decltype(f1), Scheduler> fb1(f1);
  nativefj_from_lambda<decltype(f2), Scheduler> fb2(f2);
  nativefj_from_lambda<decltype(fj), Scheduler> fbj(fj);
  auto cfb = fiber_type::current_fiber.mine();
  cfb->status = fiber_status_pause;
  fb1.outedge = &fbj; fb2.outedge = &fbj;
  fbj.outedge = cfb; cfb->incounter.store(1);
  fb2.incounter.store(0); fb2.schedule();
  fb1.incounter.store(0); fb1.schedule();
  fbj.incounter.store(2);
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

#endif

/*---------------------------------------------------------------------*/
/* Promotion mark list */

class tpalrts_prml_node {
public:
  tpalrts_prml_node* prev;
  tpalrts_prml_node* next;
};

class tpalrts_prml {
public:
  tpalrts_prml_node* front = nullptr;
  tpalrts_prml_node* back = nullptr;
  tpalrts_prml() { }
  tpalrts_prml(tpalrts_prml_node* front, tpalrts_prml_node* back)
    : front(front), back(back) { }
};

auto tpalrts_prmlist_push_back(tpalrts_prml prml, tpalrts_prml_node* t) -> tpalrts_prml {
  if (prml.back != nullptr) {
    prml.back->next = t;
  }
  prml.back = t;
  if (prml.front == nullptr) {
    prml.front = prml.back;
  }
  return prml;
}

auto tpalrts_prmlist_pop_back(tpalrts_prml prml) -> tpalrts_prml {
  auto next = prml.back;
  assert(next != nullptr);
  auto prev = next->prev;
  if (prev == nullptr) {
    prml.front = nullptr;
  } else {
    prev->next = nullptr;
    next->prev = nullptr;
  }
  prml.back = prev;
  return prml;
}

auto tpalrts_prmlist_pop_front(tpalrts_prml prml) -> tpalrts_prml {
  auto prev = prml.front;
  assert(prev != nullptr);
  auto next = prev->next;
  if (next == nullptr) {
    prml.back = nullptr;
  } else {
    next->prev = nullptr;
    prev->next = nullptr;
  }
  prml.front = next;
  return prml;
}

// To support software polling-based heartbeat by tracking the timestamp
// of the previous heartbeat
perworker::array<uint64_t> prev;

auto initialize_tpalrts() {
  for (size_t i = 0; i < prev.size(); i++) {
    prev[i] = cycles::now();
  }
}
  
} // end namespace


