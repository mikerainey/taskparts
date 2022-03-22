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
    "    mov w0, #0\n"
    "    ret\n"
);

//extern "C"
void _taskparts_ctx_restore(_context_pointer ctx, void* t) { }

#if 0

// for now, using this file as a guide:
// https://opensource.apple.com/source/xnu/xnu-4570.1.46/osfmk/arm64/cswitch.s.auto.html
// reference:
//   - $0   base pointer of context record
//   - $1   scratch register

//extern "C"
void* _taskparts_ctx_save(_context_pointer);
asm(R"(
.globl __taskparts_ctx_save
__taskparts_ctx_save:
        .cfi_startproc
        stp		x16, x17, [$0, SS64_X16]
	stp		x19, x20, [$0, SS64_X19]
	stp		x21, x22, [$0, SS64_X21]
	stp		x23, x24, [$0, SS64_X23]
	stp		x25, x26, [$0, SS64_X25]
	stp		x27, x28, [$0, SS64_X27]
	stp		fp, lr, [$0, SS64_FP]
	mov		$1, sp
	str		$1, [$0, SS64_SP]
	str		d8,	[$0, NS64_D8]
	str		d9,	[$0, NS64_D9]
	str		d10,[$0, NS64_D10]
	str		d11,[$0, NS64_D11]
	str		d12,[$0, NS64_D12]
	str		d13,[$0, NS64_D13]
	str		d14,[$0, NS64_D14]
	str		d15,[$0, NS64_D15]
        ret
        .cfi_endproc
)");

//extern "C"
void _taskparts_ctx_restore(_context_pointer ctx, void* t);
asm(R"(
.globl __taskparts_ctx_save
__taskparts_ctx_save:
        .cfi_startproc
	ldp		x16, x17, [$0, SS64_X16]
	ldp		x19, x20, [$0, SS64_X19]
	ldp		x21, x22, [$0, SS64_X21]
	ldp		x23, x24, [$0, SS64_X23]
	ldp		x25, x26, [$0, SS64_X25]
	ldp		x27, x28, [$0, SS64_X27]
	ldp		fp, lr, [$0, SS64_FP]
	ldr		$1, [$0, SS64_SP]
	mov		sp, $1
	ldr		d8,	[$0, NS64_D8]
	ldr		d9,	[$0, NS64_D9]
	ldr		d10,[$0, NS64_D10]
	ldr		d11,[$0, NS64_D11]
	ldr		d12,[$0, NS64_D12]
	ldr		d13,[$0, NS64_D13]
	ldr		d14,[$0, NS64_D14]
	ldr		d15,[$0, NS64_D15]
        .cfi_endproc
)");

#endif

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
    int _ARM_64_SP_OFFSET = 13;
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
