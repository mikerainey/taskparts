#pragma once

#include "fiber.hpp"

#if defined(TASKPARTS_X64)
#include "x64/context.hpp"
#else
#error need to declare platform (e.g., TASKPARTS_X64)
#endif

namespace taskparts {

/*---------------------------------------------------------------------*/
/* Fork join using conventional C/C++ calling conventions */

template <typename Scheduler=minimal_scheduler<>>
class nativefj_fiber : public fiber<Scheduler> {
public:

  static
  char marker1, marker2, marker3;
  
  static constexpr
  char* notaptr = &marker1;
  /* indicates to a thread that the thread does not need to deallocate
   * the call stack on which it is running
   */
  static constexpr
  char* notownstackptr = &marker2;

  static constexpr
  char* after_yield = &marker3;

  static
  perworker::array<nativefj_fiber*> current_fiber;

  fiber_status_type status = fiber_status_finish;

  // pointer to the call stack of this thread
  char* stack = nullptr;

  char* tmp_stack = nullptr;
  
  // CPU context of this thread
  context::context_type ctx;

  nativefj_fiber() : fiber<Scheduler>() { }

  ~nativefj_fiber() {
    if ((stack == nullptr) || (stack == notownstackptr)) {
      return;
    }
    assert(stack != after_yield);
    auto s = stack;
    stack = nullptr;
    free(s);
  }

  auto swap_with_scheduler() {
    context::swap(context::addr(ctx), my_ctx(), notaptr);
  }

  static
  auto exit_to_scheduler() {
    context::throw_to(my_ctx(), notaptr);
  }

  virtual
  void run2() = 0;

  auto run() -> fiber_status_type {
    run2();
    return status;
  }

  // point of entry from the scheduler to the body of this thread
  // the scheduler may reenter this fiber via this method
  auto exec() -> fiber_status_type {
    if (stack == nullptr) {
      // initial entry by the scheduler into the body of this thread
      stack = context::spawn(context::addr(ctx), this);
    }
    if (stack == after_yield) {
      status = fiber_status_finish;
      stack = tmp_stack;      
    }
    current_fiber.mine() = this;
    // jump into body of this thread
    context::swap(my_ctx(), context::addr(ctx), this);
    return status;
  }

  // point of entry to this thread to be called by the `context::spawn` routine
  static
  auto enter(nativefj_fiber* f) {
    assert(f != nullptr);
    assert(f != (nativefj_fiber*)notaptr);
    f->run();
    // terminate this fiber by exiting to scheduler
    exit_to_scheduler();
  }

  void finish() {
    fiber<Scheduler>::notify();
  }

  auto _fork1join(fiber<Scheduler>* f) {
    tmp_stack = stack;
    stack = after_yield;
    fiber<Scheduler>::add_edge(f, this);
    status = fiber_status_pause;
    f->release();
    swap_with_scheduler();
  }

  static
  auto fork1join(fiber<Scheduler>* f) {
    auto _f = current_fiber.mine();
    assert(_f != nullptr);
    _f->_fork1join(f);
  }

  auto _fork2join(nativefj_fiber* f1, nativefj_fiber* f2) {
    status = fiber_status_pause;
    fiber<Scheduler>::add_edge(f2, this);
    fiber<Scheduler>::add_edge(f1, this);
    f2->release();
    f1->release();
    if (context::capture<nativefj_fiber*>(context::addr(ctx))) {
      //aprintf("steal happened: executing join continuation\n");
      return;
    }
    // know f1 stays on my stack
    f1->stack = notownstackptr;
    f1->swap_with_scheduler();
    // sched is popping f1
    // run begin of sched->exec(f1) until f1->exec()
    f1->run();
    // if f2 was not stolen, then it can run in the same stack as parent
    auto f = Scheduler::template take<fiber>();
    if (f == nullptr) {
      status = fiber_status_finish;
      //aprintf("%d detected steal of %p\n",perworker::my_id(),f2);
      exit_to_scheduler();
      return; // unreachable
    }
    //    util::atomic::aprintf("%d %d ran %p; going to run f %p\n",id,util::worker::get_my_id(),f1,f2);
    // prepare f2 for local run
    assert(f == f2);
    assert(f2->stack == nullptr);
    f2->stack = notownstackptr;
    f2->swap_with_scheduler();
    //aprintf("ran %p and %p locally from %p\n",f1,f2,this);
    // run end of sched->exec() starting after f1->exec()
    // run begin of sched->exec(f2) until f2->exec()
    f2->run();
    status = fiber_status_finish;
    swap_with_scheduler();
    // run end of sched->exec() starting after f2->exec()
  }

  static inline
  auto fork2join(nativefj_fiber* f1, nativefj_fiber* f2) {
    auto f = current_fiber.mine();
    assert(f != nullptr);
    f->_fork2join(f1, f2);
  }
  
  template <typename Worker_reset, typename Global_reset>
  auto _reset(Worker_reset worker_reset, Global_reset global_reset) {
    fork1join(new reset_fiber(worker_reset, global_reset, Scheduler()));
  }

  template <typename Worker_reset, typename Global_reset>
  static
  void reset(Worker_reset worker_reset, Global_reset global_reset) {
    auto f = current_fiber.mine();
    assert(f != nullptr);
    f->_reset(worker_reset, global_reset);    
  }

};

template <typename Scheduler>
char nativefj_fiber<Scheduler>::marker1;

template <typename Scheduler>
char nativefj_fiber<Scheduler>::marker2;

template <typename Scheduler>
char nativefj_fiber<Scheduler>::marker3;

template <typename Scheduler>
perworker::array<nativefj_fiber<Scheduler>*> nativefj_fiber<Scheduler>::current_fiber(nullptr);
  
template <template <typename> typename Fiber, typename Scheduler=minimal_scheduler<>>
auto fork1fiberjoin(Fiber<Scheduler>* f, Scheduler sched=Scheduler()) {
  nativefj_fiber<Scheduler>::fork1join(f);
}

template <typename Worker_reset, typename Global_reset, typename Scheduler=minimal_scheduler<>>
auto reset(Worker_reset worker_reset, Global_reset global_reset, Scheduler sched=Scheduler()) {
  nativefj_fiber<Scheduler>::reset(worker_reset, global_reset);  
}

template <typename F, typename Scheduler=minimal_scheduler<>>
class nativefj_from_lambda : public nativefj_fiber<Scheduler> {
public:

  F f;

  nativefj_from_lambda(const F& f, Scheduler sched=Scheduler())
    : nativefj_fiber<Scheduler>(), f(std::move(f)) { }

  void run2() {
    f();
  }
  
};

template <typename F1, typename F2, typename Scheduler=minimal_scheduler<>>
auto fork2join(const F1& f1, const F2& f2, Scheduler sched=Scheduler()) {
  nativefj_from_lambda fb1(f1, sched);
  nativefj_from_lambda fb2(f2, sched);
  nativefj_fiber<Scheduler>::fork2join(&fb1, &fb2);
}
  
} // end namespace
