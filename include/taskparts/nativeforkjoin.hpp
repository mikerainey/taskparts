#pragma once

#include "fiber.hpp"

/*---------------------------------------------------------------------*/
/* Context switching */

using _context_pointer = char*;

extern "C"
void* _mcsl_ctx_save(_context_pointer);
asm(R"(
.globl _mcsl_ctx_save
        .type _mcsl_ctx_save, @function
        .align 16
_mcsl_ctx_save:
        .cfi_startproc
        movq %rbx, 0(%rdi)
        movq %rbp, 8(%rdi)
        movq %r12, 16(%rdi)
        movq %r13, 24(%rdi)
        movq %r14, 32(%rdi)
        movq %r15, 40(%rdi)
        leaq 8(%rsp), %rdx
        movq %rdx, 48(%rdi)
        movq (%rsp), %rax
        movq %rax, 56(%rdi)
        xorq %rax, %rax
        ret
        .size _mcsl_ctx_save, .-_mcsl_ctx_save
        .cfi_endproc
)");

extern "C"
void _mcsl_ctx_restore(_context_pointer ctx, void* t);
asm(R"(
.globl _mcsl_ctx_restore
        .type _mcsl_ctx_restore, @function
        .align 16
_mcsl_ctx_restore:
        .cfi_startproc
        movq 0(%rdi), %rbx
        movq 8(%rdi), %rbp
        movq 16(%rdi), %r12
        movq 24(%rdi), %r13
        movq 32(%rdi), %r14
        movq 40(%rdi), %r15
        test %rsi, %rsi
        mov $01, %rax
        cmove %rax, %rsi
        mov %rsi, %rax
        movq 56(%rdi), %rdx
        movq 48(%rdi), %rsp
        jmpq *%rdx
        .size _mcsl_ctx_restore, .-_mcsl_ctx_restore
        .cfi_endproc
)");

namespace taskparts {

static constexpr
std::size_t stack_alignb = 16;

static constexpr
std::size_t thread_stack_szb = stack_alignb * (1<<12);

class context {  
public:
  
  typedef char context_type[8*8];
  
  using context_pointer = _context_pointer;
  
  template <class X>
  static
  context_pointer addr(X r) {
    return r;
  }
  
  template <class Value>
  static
  void throw_to(context_pointer ctx, Value val) {
    _mcsl_ctx_restore(ctx, (void*)val);
  }
  
  template <class Value>
  static
  void swap(context_pointer ctx1, context_pointer ctx2, Value val2) {
    if (_mcsl_ctx_save(ctx1)) {
      return;
    }
    _mcsl_ctx_restore(ctx2, val2);
  }
  
  // register number 6
#define _X86_64_SP_OFFSET   6
  
  template <class Value>
  static
  Value capture(context_pointer ctx) {
    void* r = _mcsl_ctx_save(ctx);
    return (Value)r;
  }
  
  template <class Value>
  static
  char* spawn(context_pointer ctx, Value val) {
    Value target;
    if ((target = (Value)_mcsl_ctx_save(ctx))) {
      target->enter(target);
      assert(false);
    }
    char* stack = (char*)malloc(thread_stack_szb);
    char* stack_end = &stack[thread_stack_szb];
    stack_end -= (std::size_t)stack_end % stack_alignb;
    void** _ctx = (void**)ctx;    
    _ctx[_X86_64_SP_OFFSET] = stack_end;
    return stack;
  }
  
};

class context_wrapper_type {
public:
  context::context_type ctx;
};

static
perworker::array<context_wrapper_type> ctxs;

static
context::context_pointer my_ctx() {
  return context::addr(ctxs.mine().ctx);
}

/*---------------------------------------------------------------------*/
/* Fork join using conventional C/C++ calling conventions */

template <typename Scheduler>
class nativefj_fiber : public fiber<Scheduler> {
public:

  static
  char marker1, marker2;
  
  static constexpr
  char* notaptr = &marker1;
  /* indicates to a thread that the thread does not need to deallocate
   * the call stack on which it is running
   */
  static constexpr
  char* notownstackptr = &marker2;

  static
  perworker::array<nativefj_fiber*> current_fiber;

  fiber_status_type status = fiber_status_finish;

  // pointer to the call stack of this thread
  char* stack = nullptr;
  
  // CPU context of this thread
  context::context_type ctx;

  nativefj_fiber() : fiber<Scheduler>() { }

  ~nativefj_fiber() {
    if ((stack == nullptr) || (stack == notownstackptr)) {
      return;
    }
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
    current_fiber.mine() = this;
    // jump into body of this thread
    context::swap(my_ctx(), context::addr(ctx), this);
    return status;
  }

  // point of entry to this thread to be called by the `context::spawn` routine
  static
  auto enter(nativefj_fiber* t) {
    assert(t != nullptr);
    assert(t != (nativefj_fiber*)notaptr);
    t->run();
    // terminate thread by exiting to scheduler
    exit_to_scheduler();
  }

  void finish() {
    fiber<Scheduler>::notify();
  } 

  auto _fork2(nativefj_fiber* f1, nativefj_fiber* f2) {
    status = fiber_status_pause;
    fiber<Scheduler>::add_edge(f2, this);
    fiber<Scheduler>::add_edge(f1, this);
    f2->release();
    f1->release();
    if (context::capture<nativefj_fiber*>(context::addr(ctx))) {
      //      util::atomic::aprintf("steal happened: executing join continuation\n");
      return;
    }
    // know f1 stays on my stack
    f1->stack = notownstackptr;
    f1->swap_with_scheduler();
    // sched is popping f1
    // run begin of sched->exec(f1) until f1->exec()
    f1->run();
    // if f2 was not stolen, then it can run in the same stack as parent
    auto f =  Scheduler::template take<fiber>();
    if (f == nullptr) {
      status = fiber_status_finish;
      //      util::atomic::aprintf("%d %d detected steal of %p\n",id,util::worker::get_my_id(),f2);
      exit_to_scheduler();
      return; // unreachable
    }
    //    util::atomic::aprintf("%d %d ran %p; going to run f %p\n",id,util::worker::get_my_id(),f1,f2);
    // prepare f2 for local run
    assert(f == f2);
    assert(f2->stack == nullptr);
    f2->stack = notownstackptr;
    f2->swap_with_scheduler();
    //    util::atomic::aprintf("%d %d this=%p f1=%p f2=%p\n",id,util::worker::get_my_id(),this, f1, f2);
    //    printf("ran %p and %p locally\n",f1,f2);
    // run end of sched->exec() starting after f1->exec()
    // run begin of sched->exec(f2) until f2->exec()
    f2->run();
    status = fiber_status_finish;
    swap_with_scheduler();
    // run end of sched->exec() starting after f2->exec()
  }

  static
  auto fork2(nativefj_fiber* f1, nativefj_fiber* f2) {
    auto f = current_fiber.mine();
    f->_fork2(f1, f2);
  }

};

template <typename Scheduler>
char nativefj_fiber<Scheduler>::marker1;

template <typename Scheduler>
char nativefj_fiber<Scheduler>::marker2;

template <typename Scheduler>
perworker::array<nativefj_fiber<Scheduler>*> nativefj_fiber<Scheduler>::current_fiber;

template <typename F, typename Scheduler>
class nativefj_from_lambda : public nativefj_fiber<Scheduler> {
public:

  nativefj_from_lambda(const F& f) : nativefj_fiber<Scheduler>(), f(f) { }

  F f;

  void run2() {
    f();
  }
  
};

template <typename F1, typename F2, typename Scheduler=minimal_scheduler<>>
auto fork2(const F1& f1, const F2& f2) {
  nativefj_from_lambda<F1, Scheduler> fb1(f1);
  nativefj_from_lambda<F2, Scheduler> fb2(f2);
  nativefj_fiber<Scheduler>::fork2(&fb1, &fb2);
}
  
} // end namespace
