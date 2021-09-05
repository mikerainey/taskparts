#pragma once

#include <assert.h>
#include <atomic>

#include "scheduler.hpp"
#include "aligned.hpp"

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Fibers */

template <typename Scheduler>
class fiber {
private:

  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<std::size_t> incounter;

  alignas(TASKPARTS_CACHE_LINE_SZB)
  fiber* outedge;

  void schedule() {
    Scheduler::schedule(this);
  }

public:

  fiber()
    : incounter(1), outedge(nullptr) { }

  ~fiber() {
    assert(outedge == nullptr);
  }

  virtual
  fiber_status_type run() = 0;

  virtual
  fiber_status_type exec() {
    return run();
  }

  auto is_ready() -> bool {
    return incounter.load() == 0;
  }

  auto release() {
    if (--incounter == 0) {
      schedule();
    }
  }

  auto capture_continuation() -> fiber* {
    auto oe = outedge;
    outedge = nullptr;
    return oe;
  }

  static
  auto add_edge(fiber* src, fiber* dst) {
    assert(src->outedge == nullptr);
    src->outedge = dst;
    dst->incounter++;
  }

  virtual
  void notify() {
    assert(is_ready());
    auto fo = outedge;
    outedge = nullptr;
    if (fo != nullptr) {
      fo->release();
    }
  }

  virtual
  void finish() {
    notify();
    delete this;
  }

};

/* A fiber that, when executed, initiates the teardown of the 
 * scheduler. 
 */
  
template <typename Scheduler>
class terminal_fiber : public fiber<Scheduler> {
public:
  
  terminal_fiber() : fiber<Scheduler>() { }
  
  auto run() -> fiber_status_type {
    return fiber_status_exit_launch;
  }
  
};
  
} // end namespace
