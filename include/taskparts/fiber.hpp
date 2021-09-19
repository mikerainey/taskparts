#pragma once

#include <assert.h>

#include "scheduler.hpp"
#include "aligned.hpp"
#include "atomic.hpp"

namespace taskparts {
  
/*---------------------------------------------------------------------*/
/* Fibers */

template <typename Scheduler=minimal_scheduler<>>
class fiber : public minimal_fiber<Scheduler> {
private:

  alignas(TASKPARTS_CACHE_LINE_SZB)
  std::atomic<std::size_t> incounter;

  alignas(TASKPARTS_CACHE_LINE_SZB)
  fiber* outedge;

  auto schedule() {
    Scheduler::schedule(this);
  }

public:

  fiber(Scheduler _sched=Scheduler())
    : minimal_fiber<Scheduler>(), incounter(1), outedge(nullptr) {
    Scheduler::on_new_fiber();
  }

  virtual
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

  auto capture_continuation() -> fiber* {
    auto oe = outedge;
    outedge = nullptr;
    return oe;
  }

};

/*---------------------------------------------------------------------*/
/* Reset fibers */
/* We use reset fibers to reset worker-local and global memory, e.g.,
 * stats and logging. Such mechanisms require worker threads to be
 * in their busy state, and *not* performing scheduler actions, which
 * would be affecting worker-local and global scheduler memory.
 */

template <typename Worker_reset, typename Global_reset, typename Scheduler=minimal_scheduler<>>
class reset_fiber : public fiber<Scheduler> {
public:

  using trampoline_type = enum trampoline_enum {
    reset_enter,
    reset_try_to_spawn,
    reset_pause,
    reset_pause0,
    reset_wait_for_all
  };

  trampoline_type trampoline;

  std::atomic<int>* nb_workers_spawned;
  
  std::atomic<int>* nb_workers_paused;
  
  std::atomic<int>* nb_workers_finished;

  Worker_reset worker_reset;

  Global_reset global_reset;

  reset_fiber(Worker_reset worker_reset, Global_reset global_reset, Scheduler sched=Scheduler())
    : fiber<Scheduler>(),
      nb_workers_spawned(new std::atomic<int>),
      nb_workers_paused(new std::atomic<int>),
      nb_workers_finished(new std::atomic<int>),
      worker_reset(worker_reset),
      global_reset(global_reset),
      trampoline(reset_enter) { }

  reset_fiber(const reset_fiber& rf)
    :  fiber<Scheduler>(),
       nb_workers_spawned(rf.nb_workers_spawned),
       nb_workers_paused(rf.nb_workers_paused),
       nb_workers_finished(rf.nb_workers_finished),
       worker_reset(rf.worker_reset),
       global_reset(rf.global_reset),
       trampoline(reset_try_to_spawn) { }

  auto run() -> fiber_status_type {
    switch (trampoline) {
    case reset_enter: {
      if (perworker::nb_workers() == 1) {
	worker_reset();
	global_reset();
	return fiber_status_finish;
      }
      auto f = new reset_fiber(*this);
      nb_workers_spawned->store(2);
      nb_workers_paused->store(0);
      nb_workers_finished->store(0);
      f->release();
      trampoline = reset_pause0;
      return fiber_status_continue;
    }
    case reset_try_to_spawn: {
      while (true) {
	auto n = nb_workers_spawned->load();
	assert(n <= perworker::nb_workers());
	if (n == perworker::nb_workers()) {
	  trampoline = reset_pause;
	  return fiber_status_continue;
	}
	if (nb_workers_spawned->compare_exchange_strong(n, n + 1)) {
	  auto f = new reset_fiber(*this);
	  f->release();
	  trampoline = reset_pause;
	  return fiber_status_continue;
	}
      }
    }      
    case reset_pause: {
      worker_reset();
      (*nb_workers_paused)++;
      while (nb_workers_finished->load() == 0) {
	// wait for the initial fiber to finish
	busywait_pause();
      }
      (*nb_workers_finished)++;
      return fiber_status_finish;
    }
    case reset_pause0: {
      while (true) {
	if (nb_workers_paused->load() + 1 == perworker::nb_workers()) {
	  worker_reset();
	  global_reset();
	  (*nb_workers_finished)++;
	  trampoline = reset_wait_for_all;
	  return fiber_status_continue;
	}
      }
    }
    case reset_wait_for_all: {
      while (nb_workers_finished->load() != perworker::nb_workers()) {
	// wait for the other workers to finish
	busywait_pause();
      }
      delete nb_workers_spawned;
      delete nb_workers_paused;
      delete nb_workers_finished;
      return fiber_status_finish;
    }
    }
    taskparts_die("bogus");
    return fiber_status_finish;
  }
  
};

/*---------------------------------------------------------------------*/
/* Terminal fibers */
/* A fiber that, when executed, initiates the teardown of the 
 * scheduler. 
 */
  
template <typename Scheduler=minimal_scheduler<>>
class terminal_fiber : public fiber<Scheduler> {
public:
  
  terminal_fiber(Scheduler sched=Scheduler()) : fiber<Scheduler>() { }
  
  auto run() -> fiber_status_type {
    return fiber_status_exit_launch;
  }
  
};
  
} // end namespace
