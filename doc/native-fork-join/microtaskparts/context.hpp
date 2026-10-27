#pragma once

// The machine-dependent part of microtaskparts: saving and restoring CPU
// contexts. It mirrors context_save and context_restore in src/taskparts.cpp.

#include <cstddef>
#include <cstdint>

namespace microtaskparts {

// A context holds what the C ABI requires a function call to preserve: the
// callee-saved registers, the stack pointer, and the address to return to.
// Caller-saved registers need no saving: to the compiler, mtp_context_save is
// an ordinary call, so it has already spilled any live ones.
#if defined(__x86_64__)
// rbx rbp r12 r13 r14 r15 rsp rip
static constexpr int context_nb_words = 8;
static constexpr int context_sp = 6;
static constexpr int context_pc = 7;
#elif defined(__aarch64__)
// x19-x28 fp lr sp d8-d15 (the saved lr is where the context resumes)
static constexpr int context_nb_words = 21;
static constexpr int context_sp = 12;
static constexpr int context_pc = 11;
#else
#error "microtaskparts supports x86-64 and AArch64 only"
#endif

struct context {
  void *regs[context_nb_words];
};

} // namespace microtaskparts

// mtp_context_save(c) stores the caller's context in c and returns 0.
// mtp_context_restore(c) makes that call to mtp_context_save return again,
// this time with 1. This is setjmp/longjmp, except that the restoring thread
// need not be the saving one, and the context may live on any stack.
extern "C" int mtp_context_save(microtaskparts::context *c)
    __attribute__((returns_twice));
extern "C" void mtp_context_restore(microtaskparts::context *c)
    __attribute__((noreturn));

#if defined(__APPLE__)
#define MTP_SYMBOL(name) "_" #name
#define MTP_TEXT_SECTION "__TEXT,__text,regular,pure_instructions"
#else
#define MTP_SYMBOL(name) #name
#define MTP_TEXT_SECTION ".text"
#endif

// The symbols are left local (no .globl), so each translation unit that
// includes this header gets its own copy.
#if defined(__x86_64__)
asm(".pushsection " MTP_TEXT_SECTION "\n"
    ".p2align 4\n" MTP_SYMBOL(mtp_context_save) ":\n"
    "  movq %rbx, 0(%rdi)\n"
    "  movq %rbp, 8(%rdi)\n"
    "  movq %r12, 16(%rdi)\n"
    "  movq %r13, 24(%rdi)\n"
    "  movq %r14, 32(%rdi)\n"
    "  movq %r15, 40(%rdi)\n"
    "  leaq 8(%rsp), %rdx\n" // the caller's rsp, once we have returned
    "  movq %rdx, 48(%rdi)\n"
    "  movq (%rsp), %rdx\n" // our return address
    "  movq %rdx, 56(%rdi)\n"
    "  xorl %eax, %eax\n"
    "  ret\n"
    ".p2align 4\n" MTP_SYMBOL(mtp_context_restore) ":\n"
    "  movq 0(%rdi), %rbx\n"
    "  movq 8(%rdi), %rbp\n"
    "  movq 16(%rdi), %r12\n"
    "  movq 24(%rdi), %r13\n"
    "  movq 32(%rdi), %r14\n"
    "  movq 40(%rdi), %r15\n"
    "  movq 48(%rdi), %rsp\n"
    "  movl $1, %eax\n"
    "  jmpq *56(%rdi)\n"
    ".popsection\n");
#elif defined(__aarch64__)
asm(".pushsection " MTP_TEXT_SECTION "\n"
    ".p2align 4\n" MTP_SYMBOL(mtp_context_save) ":\n"
    "  stp x19, x20, [x0, #0]\n"
    "  stp x21, x22, [x0, #16]\n"
    "  stp x23, x24, [x0, #32]\n"
    "  stp x25, x26, [x0, #48]\n"
    "  stp x27, x28, [x0, #64]\n"
    "  stp x29, x30, [x0, #80]\n"
    "  mov x9, sp\n"
    "  str x9, [x0, #96]\n"
    "  stp d8, d9, [x0, #104]\n"
    "  stp d10, d11, [x0, #120]\n"
    "  stp d12, d13, [x0, #136]\n"
    "  stp d14, d15, [x0, #152]\n"
    "  mov w0, #0\n"
    "  ret\n"
    ".p2align 4\n" MTP_SYMBOL(mtp_context_restore) ":\n"
    "  ldp x19, x20, [x0, #0]\n"
    "  ldp x21, x22, [x0, #16]\n"
    "  ldp x23, x24, [x0, #32]\n"
    "  ldp x25, x26, [x0, #48]\n"
    "  ldp x27, x28, [x0, #64]\n"
    "  ldp x29, x30, [x0, #80]\n"
    "  ldr x9, [x0, #96]\n"
    "  mov sp, x9\n"
    "  ldp d8, d9, [x0, #104]\n"
    "  ldp d10, d11, [x0, #120]\n"
    "  ldp d12, d13, [x0, #136]\n"
    "  ldp d14, d15, [x0, #152]\n"
    "  mov w0, #1\n"
    "  ret\n" // to the saved lr
    ".popsection\n");
#endif

namespace microtaskparts {

// Make c a context that, once restored, calls entry() at the top of the given
// stack. entry must never return.
inline void context_init(context &c, char *stack, size_t stack_szb,
                         void (*entry)()) {
  auto sp = reinterpret_cast<uintptr_t>(stack + stack_szb) & ~uintptr_t(15);
  c = context{};
#if defined(__x86_64__)
  // Push a null return address, as if a call instruction had entered entry().
  sp -= sizeof(void *);
  *reinterpret_cast<void **>(sp) = nullptr;
#endif
  c.regs[context_sp] = reinterpret_cast<void *>(sp);
  c.regs[context_pc] = reinterpret_cast<void *>(entry);
}

} // namespace microtaskparts
