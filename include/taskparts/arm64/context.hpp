#pragma once

#include "../perworker.hpp"

/*---------------------------------------------------------------------*/
/* Context switching */

static constexpr
int arm64_ctx_szb = 13*8;

using _context_pointer = char*;

extern "C"
void* _taskparts_ctx_save(_context_pointer);
__asm__
(
    "__taskparts_ctx_save:\n"
    "    str x19, [x0, 0]\n"
    "    str x20, [x0, 1]\n"
    "    str x21, [x0, 2]\n"
    "    str x22, [x0, 3]\n"
    "    str x23, [x0, 4]\n"
    "    str x24, [x0, 5]\n"
    "    str x25, [x0, 6]\n"
    "    str x26, [x0, 7]\n"
    "    str x27, [x0, 8]\n"
    "    str x28, [x0, 9]\n"
    "    str x29, [x0, 10]\n"
    "    str x30, [x0, 11]\n"
    "    str x31, [x0, 12]\n"
    "    mov x0, #0\n"
    "    ret\n"
);

extern "C"
void _taskparts_ctx_restore(_context_pointer ctx, void* t);
__asm__
(
    "__taskparts_ctx_restore:\n"
    "    ldr x31, [x0, 12]\n"
    "    ldr x30, [x0, 11]\n"
    "    ldr x29, [x0, 10]\n"
    "    ldr x28, [x0, 9]\n"
    "    ldr x27, [x0, 8]\n"
    "    ldr x26, [x0, 7]\n"
    "    ldr x25, [x0, 6]\n"
    "    ldr x24, [x0, 5]\n"
    "    ldr x23, [x0, 4]\n"
    "    ldr x22, [x0, 3]\n"
    "    ldr x21, [x0, 2]\n"
    "    ldr x20, [x0, 1]\n"
    "    ldr x19, [x0, 0]\n"
    "    mov x2, #1\n"
    "    cmp x1, #0\n"
    "    csel x0, x2, x1, eq\n"
    "    ret\n"
);

namespace taskparts {

static constexpr
size_t stack_alignb = 16;

static constexpr
size_t thread_stack_szb = stack_alignb * (1<<12);

class context {  
public:
  
  typedef char context_type[arm64_ctx_szb];
  
  using context_pointer = _context_pointer;
  
  template <class X>
  static
  context_pointer addr(X r) {
    return r;
  }
  
  template <class Value>
  static
  void throw_to(context_pointer ctx, Value val) {
    _taskparts_ctx_restore(ctx, (void*)val);
  }
  
  template <class Value>
  static
  void swap(context_pointer ctx1, context_pointer ctx2, Value val2) {
    if (_taskparts_ctx_save(ctx1)) {
      return;
    }
    _taskparts_ctx_restore(ctx2, val2);
  }
  
  template <class Value>
  static
  Value capture(context_pointer ctx) {
    void* r = _taskparts_ctx_save(ctx);
    return (Value)r;
  }
  
  template <class Value>
  static
  char* spawn(context_pointer ctx, Value val) {
    Value target;
    if ((target = (Value)_taskparts_ctx_save(ctx))) {
      target->enter(target);
      assert(false);
    }
    char* stack = (char*)malloc(thread_stack_szb);
    char* stack_end = &stack[thread_stack_szb];
    stack_end -= (size_t)stack_end % stack_alignb;
    void** _ctx = (void**)ctx;
    static constexpr
    int _ARM_64_SP_OFFSET = 12;
    _ctx[_ARM_64_SP_OFFSET] = stack_end;
    return stack;
  }
  
};

class context_wrapper_type {
public:
  context::context_type ctx;
};

perworker::array<context_wrapper_type> ctxs;

context::context_pointer my_ctx() {
  return context::addr(ctxs.mine().ctx);
}

} // end namespace
