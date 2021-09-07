_Z16sum_array_serialPdmm:
        vxorpd  %xmm0, %xmm0, %xmm0
        cmpq    %rdx, %rsi
        je      .LBB0_3
        vxorpd  %xmm0, %xmm0, %xmm0
.LBB0_2:
        vaddsd  (%rdi,%rsi,8), %xmm0, %xmm0
        incq    %rsi
        cmpq    %rsi, %rdx
        jne     .LBB0_2
.LBB0_3:
        retq

_Z19sum_array_heartbeatPdmmdS_:
        pushq   %r15
        pushq   %r14
        pushq   %r12
        pushq   %rbx
        pushq   %rax
        movq    %rcx, %r14
        cmpq    %rdx, %rsi
        jae     .LBB1_8
        movq    %rdx, %r15
        movq    %rsi, %rbx
        movq    %rdi, %r12
.LBB1_2:
        leaq    64(%rbx), %rax
        cmpq    %r15, %rax
        cmovaq  %r15, %rax
        cmpq    %rax, %rbx
        jae     .LBB1_5
.LBB1_3:
        vaddsd  (%r12,%rbx,8), %xmm0, %xmm0
        incq    %rbx
        cmpq    %rbx, %rax
        jne     .LBB1_3
        movq    %rax, %rbx
.LBB1_5:
        cmpq    %r15, %rbx
        jae     .LBB1_8
        cmpl    $0, heartbeat(%rip)
        je      .LBB1_2
        vmovsd  %xmm0, (%rsp)
        movq    %r12, %rdi
        movq    %rbx, %rsi
        movq    %r15, %rdx
        movq    %r14, %rcx
        vmovsd  (%rsp), %xmm0
        callq   _Z27sum_array_heartbeat_handlerPdmmdS_
        vmovsd  (%rsp), %xmm0
        testl   %eax, %eax
        je      .LBB1_2
        jmp     .LBB1_9
.LBB1_8:
        vmovsd  %xmm0, (%r14)
.LBB1_9:
        addq    $8, %rsp
        popq    %rbx
        popq    %r12
        popq    %r14
        popq    %r15
        retq
