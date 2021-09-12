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
        movq    (%rdi), %rax
        addq    $24, %rax
        movq    %rdi, %rcx
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
        subq    %rbx, %rax
        movabsq $9223372036854775800, %rcx
        cmpq    %rcx, %rax
        je      .LBB0_20
        movq    %rax, %rbp
        sarq    $3, %rbp
        movabsq $-6148914691236517205, %rcx
        imulq   %rcx, %rbp
        testq   %rax, %rax
        movq    %rbp, %rax
        movl    $1, %ecx
        cmoveq  %rcx, %rax
        leaq    (%rax,%rbp), %r14
        movabsq $384307168202282325, %rcx
        cmpq    %rcx, %r14
        cmovaq  %rcx, %r14
        addq    %rbp, %rax
        cmovbq  %rcx, %r14
        leaq    (,%r14,8), %rax
        leaq    (%rax,%rax,2), %rdi
        callq   _Znwm
        leaq    (,%rbp,2), %rcx
        addq    %rbp, %rcx
        movq    %rax, %rbp
        movl    $0, (%rax,%rcx,8)
        movq    %r15, 8(%rax,%rcx,8)
        cmpq    %r13, %rbx
        movq    %rbx, %rdi
        movq    %rax, %r12
        je      .LBB0_15
        movq    %rdi, %rax
.LBB0_14:
        movq    16(%rax), %rcx
        movq    %rcx, 16(%r12)
        movups  (%rax), %xmm0
        movups  %xmm0, (%r12)
        addq    $24, %rax
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
        movq    %rbp, (%rsi)
        movq    %r12, 8(%rsi)
        leaq    (%r14,%r14,2), %rax
        leaq    (,%rax,8), %rax
        addq    %rbp, %rax
        movq    (%rsp), %rbp
        movq    %rbp, %rcx
        movq    8(%rsp), %rdi
        jmp     .LBB0_18
.LBB0_2:
        xorl    %eax, %eax
.LBB0_3:
        leaq    -24(%r13), %rcx
        movl    -24(%r13), %edx
.LBB0_4:
        testl   %edx, %edx
        je      .LBB0_8
        cmpl    $2, %edx
        je      .LBB0_19
        cmpl    $1, %edx
        jne     .LBB0_4
        addl    -16(%r13), %eax
        movq    -8(%r13), %rdx
        addl    (%rdx), %eax
        movq    %rcx, (%rdi)
        movq    %rcx, %r13
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
        callq   _Znwm
        movq    %rax, (%rsp)
        leaq    24(%rax), %rcx
        movq    %rcx, 16(%rsp)
        movl    $2, (%rax)
        movl    44(%rsp), %edx
        movl    %edx, 20(%rax)
        movups  28(%rsp), %xmm0
        movups  %xmm0, 4(%rax)
        movq    %rcx, 8(%rsp)
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
        movq    %rax, %rbx
        movq    (%rsp), %rdi
        testq   %rdi, %rdi
        je      .LBB1_6
        callq   _ZdlPv
.LBB1_6:
        movq    %rbx, %rdi
        callq   _Unwind_Resume@PLT

_Z17prmlist_push_backRSt6vectorI7vhbkontSaIS0_EEiii:
        movl    %esi, %eax
        cmpl    $-1, %edx
        je      .LBB2_2
        movslq  %edx, %rdx
        movq    (%rdi), %rsi
        shlq    $5, %rdx
        movl    %ecx, 20(%rsi,%rdx)
.LBB2_2:
        cmpl    $-1, %eax
        cmovel  %ecx, %eax
        shlq    $32, %rcx
        orq     %rcx, %rax
        retq

_Z16prmlist_pop_backRSt6vectorI7vhbkontSaIS0_EEii:
        movslq  %edx, %rcx
        movq    (%rdi), %r8
        shlq    $5, %rcx
        movslq  16(%r8,%rcx), %rax
        cmpq    $-1, %rax
        je      .LBB3_1
        movq    %rax, %rdx
        shlq    $5, %rdx
        movl    $-1, 20(%r8,%rdx)
        movq    (%rdi), %rdx
        movl    $-1, 16(%rdx,%rcx)
        movl    %esi, %ecx
        shlq    $32, %rax
        orq     %rcx, %rax
        retq
.LBB3_1:
        movl    $4294967295, %ecx
        shlq    $32, %rax
        orq     %rcx, %rax
        retq

_Z13sum_heartbeatP4nodeRSt6vectorI7vhbkontSaIS2_EEii:
        pushq   %rbp
        pushq   %r15
        pushq   %r14
        pushq   %r13
        pushq   %r12
        pushq   %rbx
        subq    $24, %rsp
        movl    %ecx, %ebx
        movl    %edx, %r14d
        movq    %rsi, %r15
.LBB4_1:
        cmpb    $0, heartbeat(%rip)
        je      .LBB4_2
        movq    %rdi, %rbp
        movq    %r15, %rdi
        movl    %r14d, %esi
        movl    %ebx, %edx
        callq   _Z26sum_tree_heartbeat_handlerRSt6vectorI7vhbkontSaIS0_EEii
        movq    %rbp, %rdi
        movq    %rax, %r14
        movq    %rax, %r13
        shrq    $32, %r13
        testq   %rdi, %rdi
        je      .LBB4_5
.LBB4_26:
        movq    8(%r15), %r12
        cmpq    16(%r15), %r12
        je      .LBB4_28
        movl    $0, (%r12)
        movq    %rdi, 8(%r12)
        movl    %r13d, 16(%r12)
        movl    $-1, 20(%r12)
        movq    (%r15), %rbp
        movq    8(%r15), %rbx
        addq    $32, %rbx
        movq    %rbx, 8(%r15)
        jmp     .LBB4_38
.LBB4_2:
        movl    %ebx, %r13d
        testq   %rdi, %rdi
        jne     .LBB4_26
.LBB4_5:
        cmpb    $0, heartbeat(%rip)
        je      .LBB4_11
        xorl    %ebp, %ebp
        movb    $1, %al
        jmp     .LBB4_7
.LBB4_22:
        leaq    -32(%rax), %rcx
        addl    -24(%rax), %ebp
        movq    -16(%rax), %rax
        addl    (%rax), %ebp
        movq    %rcx, 8(%r15)
.LBB4_23:
        movzbl  heartbeat(%rip), %eax
.LBB4_7:
        testb   %al, %al
        je      .LBB4_9
        movq    %r15, %rdi
        movl    %r14d, %esi
        movl    %r13d, %edx
        callq   _Z26sum_tree_heartbeat_handlerRSt6vectorI7vhbkontSaIS0_EEii
        movq    %rax, %r14
        movq    %rax, %r13
        shrq    $32, %r13
.LBB4_9:
        movq    8(%r15), %rax
        movl    -32(%rax), %ecx
        cmpq    $4, %rcx
        ja      .LBB4_23
        jmpq    *.LJTI4_0(,%rcx,8)
.LBB4_16:
        movslq  %r13d, %rcx
        movq    (%r15), %rdx
        shlq    $5, %rcx
        movslq  16(%rdx,%rcx), %rbx
        cmpq    $-1, %rbx
        je      .LBB4_17
        movq    %rbx, %rsi
        shlq    $5, %rsi
        movl    $-1, 20(%rdx,%rsi)
        movq    (%r15), %rdx
        movl    $-1, 16(%rdx,%rcx)
        jmp     .LBB4_25
.LBB4_28:
        movq    (%r15), %rdx
        movq    %r12, %rax
        subq    %rdx, %rax
        movabsq $9223372036854775776, %rcx
        cmpq    %rcx, %rax
        je      .LBB4_41
        movq    %rax, %rbx
        sarq    $5, %rbx
        testq   %rax, %rax
        movq    %rbx, %rax
        movl    $1, %ecx
        cmoveq  %rcx, %rax
        leaq    (%rax,%rbx), %rsi
        movq    %rsi, %rcx
        shrq    $58, %rcx
        movabsq $288230376151711743, %rcx
        cmovneq %rcx, %rsi
        addq    %rbx, %rax
        cmovbq  %rcx, %rsi
        testq   %rsi, %rsi
        movq    %rsi, 16(%rsp)
        je      .LBB4_30
        movq    %rdi, 8(%rsp)
        movq    %rsi, %rdi
        shlq    $5, %rdi
        movq    %rdx, %rbp
        callq   _Znwm
        movq    %rbp, %rdx
        movq    8(%rsp), %rdi
        movq    %rax, %rbp
        jmp     .LBB4_32
.LBB4_11:
        movq    8(%r15), %rax
        xorl    %ebp, %ebp
.LBB4_12:
        leaq    -32(%rax), %rdx
        movl    -32(%rax), %ecx
.LBB4_13:
        cmpl    $4, %ecx
        ja      .LBB4_13
        movl    %ecx, %esi
        jmpq    *.LJTI4_1(,%rsi,8)
.LBB4_15:
        addl    -24(%rax), %ebp
        movq    -16(%rax), %rax
        addl    (%rax), %ebp
        movq    %rdx, 8(%r15)
        movq    %rdx, %rax
        jmp     .LBB4_12
.LBB4_30:
        xorl    %ebp, %ebp
.LBB4_32:
        shlq    $5, %rbx
        movl    $0, (%rbp,%rbx)
        movq    %rdi, 8(%rbp,%rbx)
        movl    %r13d, 16(%rbp,%rbx)
        movl    $-1, 20(%rbp,%rbx)
        movq    %rbp, %rbx
        cmpq    %r12, %rdx
        je      .LBB4_35
        movq    %rbp, %rbx
        movq    %rdx, %rax
.LBB4_34:
        movups  (%rax), %xmm0
        movups  16(%rax), %xmm1
        movups  %xmm1, 16(%rbx)
        movups  %xmm0, (%rbx)
        addq    $32, %rax
        addq    $32, %rbx
        cmpq    %rax, %r12
        jne     .LBB4_34
.LBB4_35:
        addq    $32, %rbx
        testq   %rdx, %rdx
        je      .LBB4_37
        movq    %rdi, %r12
        movq    %rdx, %rdi
        callq   _ZdlPv
        movq    %r12, %rdi
.LBB4_37:
        movq    %rbp, (%r15)
        movq    %rbx, 8(%r15)
        movq    16(%rsp), %rax
        shlq    $5, %rax
        addq    %rbp, %rax
        movq    %rax, 16(%r15)
.LBB4_38:
        subq    %rbp, %rbx
        shrq    $5, %rbx
        addl    $-1, %ebx
        cmpl    $-1, %r13d
        je      .LBB4_40
        movslq  %r13d, %rax
        shlq    $5, %rax
        movl    %ebx, 20(%rbp,%rax)
.LBB4_40:
        cmpl    $-1, %r14d
        cmovel  %ebx, %r14d
        movq    8(%rdi), %rdi
        jmp     .LBB4_1
.LBB4_17:
        movl    $-1, %r14d
.LBB4_25:
        movq    -24(%rax), %rcx
        movq    16(%rcx), %rdi
        movl    $1, -32(%rax)
        movl    %ebp, -24(%rax)
        movq    %rcx, -16(%rax)
        jmp     .LBB4_1
.LBB4_19:
        xorl    %edx, %edx
        cmpl    $3, %ecx
        setne   %dl
        movq    -24(%rax), %rcx
        movl    %ebp, (%rcx,%rdx,4)
        movq    -16(%rax), %rdi
        jne     .LBB4_21
        movq    -8(%rax), %rcx
        movups  (%rcx), %xmm0
        movq    16(%rcx), %rdx
        movq    (%r15), %rsi
        movq    %rsi, (%rcx)
        movq    %rax, 8(%rcx)
        movq    16(%r15), %rax
        movq    %rax, 16(%rcx)
        movups  %xmm0, (%r15)
        movq    %rdx, 16(%r15)
.LBB4_21:
        addq    $24, %rsp
        popq    %rbx
        popq    %r12
        popq    %r13
        popq    %r14
        popq    %r15
        popq    %rbp
        jmp     _Z4joinPv
.LBB4_18:
        movl    %ebp, answer(%rip)
        addq    $24, %rsp
        popq    %rbx
        popq    %r12
        popq    %r13
        popq    %r14
        popq    %r15
        popq    %rbp
        retq
.LBB4_41:
        movl    $.L.str, %edi
        callq   _ZSt20__throw_length_errorPKc
.LJTI4_0:
        .quad   .LBB4_16
        .quad   .LBB4_22
        .quad   .LBB4_18
        .quad   .LBB4_19
        .quad   .LBB4_19
.LJTI4_1:
        .quad   .LBB4_16
        .quad   .LBB4_15
        .quad   .LBB4_18
        .quad   .LBB4_19
        .quad   .LBB4_19

.L.str:
        .asciz  "vector::_M_realloc_insert"
	
