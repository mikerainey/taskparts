_Z3sumP4nodeRSt6vectorI5vkontSaIS2_EE:
        pushq   %rbp
        pushq   %r15
        pushq   %r14
        pushq   %r13
        pushq   %r12
        pushq   %rbx
        subq    $24, %rsp
        movq    %rdi, %r15
        leaq    8(%rsi), %rdi
        leaq    16(%rsi), %rbp
        movq    %rsi, 16(%rsp)
        movq    %rdi, 8(%rsp)
        movq    %rbp, (%rsp)
        movq    (%rdi), %r13
        testq   %r15, %r15
        jne     .LBB0_9
        jmp     .LBB0_2
.LBB0_10:
        movl    $0, (%r13)
        movq    %r15, 8(%r13)
        movq    %rdi, %rcx
        movq    (%rdi), %rax
        addq    $24, %rax
.LBB0_18:
        movq    %rax, (%rcx)
        movq    8(%r15), %r15
        movq    (%rdi), %r13
        testq   %r15, %r15
        je      .LBB0_2
.LBB0_9:
        cmpq    (%rbp), %r13
        jne     .LBB0_10
        movq    (%rsi), %rbx
        movq    %r13, %rax
        movabsq $9223372036854775800, %rcx
        subq    %rbx, %rax
        cmpq    %rcx, %rax
        je      .LBB0_20
        movq    %rax, %rbp
        movabsq $-6148914691236517205, %rcx
        sarq    $3, %rbp
        imulq   %rcx, %rbp
        testq   %rax, %rax
        movl    $1, %ecx
        movq    %rbp, %rax
        cmoveq  %rcx, %rax
        movabsq $384307168202282325, %rcx
        leaq    (%rax,%rbp), %r14
        cmpq    %rcx, %r14
        cmovaq  %rcx, %r14
        addq    %rbp, %rax
        cmovbq  %rcx, %r14
        leaq    (,%r14,8), %rax
        leaq    (%rax,%rax,2), %rdi
        callq   _Znwm
        leaq    (%rbp,%rbp,2), %rcx
        movq    %rax, %rbp
        movq    %rbx, %rdi
        movq    %rax, %r12
        movl    $0, (%rax,%rcx,8)
        movq    %r15, 8(%rax,%rcx,8)
        cmpq    %r13, %rbx
        je      .LBB0_15
        movq    %rdi, %rax
.LBB0_14:
        movq    16(%rax), %rcx
        movq    %rcx, 16(%r12)
        vmovups (%rax), %xmm0
        addq    $24, %rax
        vmovups %xmm0, (%r12)
        addq    $24, %r12
        cmpq    %rax, %r13
        jne     .LBB0_14
.LBB0_15:
        addq    $24, %r12
        testq   %rdi, %rdi
        je      .LBB0_17
        callq   _ZdlPv
.LBB0_17:
        movq    16(%rsp), %rsi
        leaq    (%r14,%r14,2), %rax
        movq    8(%rsp), %rdi
        leaq    (%rbp,%rax,8), %rax
        movq    %rbp, (%rsi)
        movq    (%rsp), %rbp
        movq    %r12, 8(%rsi)
        movq    %rbp, %rcx
        jmp     .LBB0_18
.LBB0_2:
        xorl    %eax, %eax
.LBB0_3:
        movl    -24(%r13), %edx
        leaq    -24(%r13), %rcx
.LBB0_4:
        testl   %edx, %edx
        je      .LBB0_8
        cmpl    $2, %edx
        je      .LBB0_19
        cmpl    $1, %edx
        jne     .LBB0_4
        movq    -8(%r13), %rdx
        addl    -16(%r13), %eax
        movq    %rcx, %r13
        addl    (%rdx), %eax
        movq    %rcx, (%rdi)
        jmp     .LBB0_3
.LBB0_8:
        movq    -16(%r13), %rcx
        movq    16(%rcx), %r15
        movl    $1, -24(%r13)
        movl    %eax, -16(%r13)
        movq    %rcx, -8(%r13)
        movq    (%rdi), %r13
        testq   %r15, %r15
        jne     .LBB0_9
        jmp     .LBB0_2
.LBB0_19:
        movl    %eax, answer(%rip)
        addq    $24, %rsp
        popq    %rbx
        popq    %r12
        popq    %r13
        popq    %r14
        popq    %r15
        popq    %rbp
        retq
.LBB0_20:
        movl    $.L.str, %edi
        callq   _ZSt20__throw_length_errorPKc

_Z10sum_serialP4node:
        pushq   %rbx
        subq    $48, %rsp
        movq    %rdi, %rbx
        movl    $24, %edi
        movl    $3, answer(%rip)
        callq   _Znwm
        movl    44(%rsp), %edx
        vmovups 28(%rsp), %xmm0
        leaq    24(%rax), %rcx
        movl    $2, (%rax)
        movq    %rax, (%rsp)
        movq    %rcx, 16(%rsp)
        movq    %rcx, 8(%rsp)
        movl    %edx, 20(%rax)
        vmovups %xmm0, 4(%rax)
        movq    %rsp, %rsi
        movq    %rbx, %rdi
        callq   _Z3sumP4nodeRSt6vectorI5vkontSaIS2_EE
        movq    (%rsp), %rdi
        testq   %rdi, %rdi
        je      .LBB1_3
        callq   _ZdlPv
.LBB1_3:
        addq    $48, %rsp
        popq    %rbx
        retq
        movq    (%rsp), %rdi
        movq    %rax, %rbx
        testq   %rdi, %rdi
        je      .LBB1_6
        callq   _ZdlPv
.LBB1_6:
        movq    %rbx, %rdi
        callq   _Unwind_Resume@PLT

.L.str:
        .asciz  "vector::_M_realloc_insert"
