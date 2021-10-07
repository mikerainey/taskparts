	.text
	.file	"spmv_orig.cpp"
	.globl	_Z14spmv_interruptPfPmS0_S_S_mm
	.p2align	4, 0x90
	.type	_Z14spmv_interruptPfPmS0_S_S_mm,@function
_Z14spmv_interruptPfPmS0_S_S_mm:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	subq	$72, %rsp
	.cfi_def_cfa_offset 128
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpq	128(%rsp), %r9
	jae	.LBB0_17
	movq	%rcx, %rbx
	leaq	96(%rdx), %rcx
	movq	%rdx, %r12
	movq	heartbeat@GOTPCREL(%rip), %rdx
	leaq	48(%rdi), %rax
	movq	%rdi, %r15
	vxorps	%xmm9, %xmm9, %xmm9
	movq	%rax, 40(%rsp)
	movq	%rcx, 32(%rsp)
	.p2align	4, 0x90
.LBB0_2:
	movq	(%rsi,%r9,8), %r14
	movq	8(%rsi,%r9,8), %r13
	vxorps	%xmm0, %xmm0, %xmm0
	cmpq	%r13, %r14
	jae	.LBB0_14
	movq	%r13, %rdi
	vxorps	%xmm0, %xmm0, %xmm0
	notq	%rdi
	movq	%rdi, 8(%rsp)
	.p2align	4, 0x90
.LBB0_4:
	leaq	1024(%r14), %rax
	cmpq	%rax, %r13
	cmovbq	%r13, %rax
	cmpq	%rax, %r14
	jae	.LBB0_11
	movq	$-1025, %rcx
	movq	%r14, %r10
	subq	%r14, %rcx
	notq	%r10
	cmpq	%rdi, %rcx
	cmovbq	%rdi, %rcx
	subq	%rcx, %r10
	cmpq	$16, %r10
	jb	.LBB0_9
	movq	40(%rsp), %rcx
	movq	%r10, %r11
	vblendps	$1, %xmm0, %xmm9, %xmm8
	vxorps	%xmm1, %xmm1, %xmm1
	vxorps	%xmm2, %xmm2, %xmm2
	vxorps	%xmm3, %xmm3, %xmm3
	andq	$-16, %r11
	leaq	(%rcx,%r14,4), %rdx
	movq	32(%rsp), %rcx
	leaq	(%rcx,%r14,8), %rdi
	leaq	(%r14,%r11), %r14
	xorl	%ecx, %ecx
	.p2align	4, 0x90
.LBB0_7:
	vmovdqu	-96(%rdi,%rcx,8), %ymm6
	vmovdqu	-64(%rdi,%rcx,8), %ymm7
	vmovdqu	-32(%rdi,%rcx,8), %ymm5
	vmovdqu	(%rdi,%rcx,8), %ymm4
	vmovq	%xmm6, %rbp
	vmovss	(%rbx,%rbp,4), %xmm0
	vpextrq	$1, %xmm6, %rbp
	vextracti128	$1, %ymm6, %xmm6
	vinsertps	$16, (%rbx,%rbp,4), %xmm0, %xmm0
	vmovq	%xmm6, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm0, %xmm0
	vpextrq	$1, %xmm6, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm0, %xmm0
	vmovq	%xmm7, %rbp
	vmovss	(%rbx,%rbp,4), %xmm6
	vpextrq	$1, %xmm7, %rbp
	vextracti128	$1, %ymm7, %xmm7
	vmulps	-48(%rdx,%rcx,4), %xmm0, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm6, %xmm6
	vmovq	%xmm7, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm6, %xmm6
	vpextrq	$1, %xmm7, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm6, %xmm6
	vmovq	%xmm5, %rbp
	vaddps	%xmm0, %xmm8, %xmm8
	vmovss	(%rbx,%rbp,4), %xmm7
	vpextrq	$1, %xmm5, %rbp
	vextracti128	$1, %ymm5, %xmm5
	vmulps	-32(%rdx,%rcx,4), %xmm6, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm7, %xmm7
	vmovq	%xmm5, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm5, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm7, %xmm5
	vmovq	%xmm4, %rbp
	vaddps	%xmm0, %xmm1, %xmm1
	vmovss	(%rbx,%rbp,4), %xmm7
	vpextrq	$1, %xmm4, %rbp
	vextracti128	$1, %ymm4, %xmm4
	vmulps	-16(%rdx,%rcx,4), %xmm5, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm7, %xmm7
	vmovq	%xmm4, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm4, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm7, %xmm4
	vaddps	%xmm0, %xmm2, %xmm2
	vmulps	(%rdx,%rcx,4), %xmm4, %xmm4
	addq	$16, %rcx
	cmpq	%rcx, %r11
	vaddps	%xmm4, %xmm3, %xmm3
	jne	.LBB0_7
	vaddps	%xmm8, %xmm1, %xmm0
	movq	heartbeat@GOTPCREL(%rip), %rdx
	movq	8(%rsp), %rdi
	cmpq	%r11, %r10
	vaddps	%xmm0, %xmm2, %xmm0
	vaddps	%xmm0, %xmm3, %xmm0
	vpermilpd	$1, %xmm0, %xmm1
	vaddps	%xmm1, %xmm0, %xmm0
	vhaddps	%xmm0, %xmm0, %xmm0
	je	.LBB0_10
	.p2align	4, 0x90
.LBB0_9:
	movq	(%r12,%r14,8), %rcx
	vmovss	(%r15,%r14,4), %xmm1
	incq	%r14
	cmpq	%rax, %r14
	vmulss	(%rbx,%rcx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	jb	.LBB0_9
.LBB0_10:
	movq	%rax, %r14
.LBB0_11:
	cmpq	%r13, %r14
	jae	.LBB0_14
	cmpl	$0, (%rdx)
	je	.LBB0_4
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movq	%r15, %rdi
	movq	%r12, %rdx
	movq	%rbx, %rcx
	movq	%r9, %rbp
	pushq	%r13
	.cfi_adjust_cfa_offset 8
	pushq	%r14
	.cfi_adjust_cfa_offset 8
	pushq	152(%rsp)
	.cfi_adjust_cfa_offset 8
	movq	%r8, 56(%rsp)
	movq	%rsi, 48(%rsp)
	vmovaps	%xmm0, 80(%rsp)
	vzeroupper
	callq	_Z16col_loop_handlerPfPmS0_S_S_mmmmf@PLT
	vmovaps	80(%rsp), %xmm0
	movq	40(%rsp), %rdi
	movq	heartbeat@GOTPCREL(%rip), %rdx
	movq	48(%rsp), %rsi
	movq	56(%rsp), %r8
	vxorps	%xmm9, %xmm9, %xmm9
	movq	%rbp, %r9
	addq	$32, %rsp
	.cfi_adjust_cfa_offset -32
	testl	%eax, %eax
	je	.LBB0_4
	jmp	.LBB0_17
	.p2align	4, 0x90
.LBB0_14:
	vmovss	%xmm0, (%r8,%r9,4)
	incq	%r9
	cmpq	128(%rsp), %r9
	jae	.LBB0_17
	cmpl	$0, (%rdx)
	je	.LBB0_2
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movq	%r12, %rdx
	movq	%r15, %rdi
	movq	%rbx, %rcx
	movq	%r9, %rbp
	pushq	136(%rsp)
	.cfi_adjust_cfa_offset 8
	movq	%r8, %r14
	movq	%rsi, %r13
	vzeroupper
	callq	_Z16row_loop_handlerPfPmS0_S_S_mm@PLT
	movq	heartbeat@GOTPCREL(%rip), %rdx
	vxorps	%xmm9, %xmm9, %xmm9
	movq	%r13, %rsi
	movq	%r14, %r8
	movq	%rbp, %r9
	addq	$16, %rsp
	.cfi_adjust_cfa_offset -16
	testl	%eax, %eax
	je	.LBB0_2
.LBB0_17:
	addq	$72, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%r12
	.cfi_def_cfa_offset 40
	popq	%r13
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	vzeroupper
	retq
.Lfunc_end0:
	.size	_Z14spmv_interruptPfPmS0_S_S_mm, .Lfunc_end0-_Z14spmv_interruptPfPmS0_S_S_mm
	.cfi_endproc

	.globl	_Z23spmv_interrupt_col_loopPfPmS0_S_S_mmfS_
	.p2align	4, 0x90
	.type	_Z23spmv_interrupt_col_loopPfPmS0_S_S_mmfS_,@function
_Z23spmv_interrupt_col_loopPfPmS0_S_S_mmfS_:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	subq	$56, %rsp
	.cfi_def_cfa_offset 112
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movq	112(%rsp), %r14
	movq	120(%rsp), %r10
	cmpq	%r14, %r9
	jae	.LBB1_12
	leaq	48(%rdi), %rax
	movq	%rdi, %r12
	movq	heartbeat@GOTPCREL(%rip), %rdi
	movq	%rdx, %r15
	movq	%r14, %rdx
	movq	%r9, %r13
	movq	%rcx, %rbx
	vxorps	%xmm9, %xmm9, %xmm9
	notq	%rdx
	movq	%rax, 24(%rsp)
	leaq	96(%r15), %rax
	movq	%rax, 16(%rsp)
	movq	%rdx, (%rsp)
	.p2align	4, 0x90
.LBB1_2:
	leaq	1024(%r13), %rax
	cmpq	%r14, %rax
	cmovaq	%r14, %rax
	cmpq	%rax, %r13
	jae	.LBB1_9
	movq	$-1025, %rcx
	movq	%r13, %r9
	subq	%r13, %rcx
	notq	%r9
	cmpq	%rdx, %rcx
	cmovbq	%rdx, %rcx
	subq	%rcx, %r9
	cmpq	$16, %r9
	jb	.LBB1_7
	movq	24(%rsp), %rcx
	movq	%r9, %r11
	vblendps	$1, %xmm0, %xmm9, %xmm8
	vxorps	%xmm1, %xmm1, %xmm1
	vxorps	%xmm2, %xmm2, %xmm2
	vxorps	%xmm3, %xmm3, %xmm3
	andq	$-16, %r11
	leaq	(%rcx,%r13,4), %rdx
	movq	16(%rsp), %rcx
	leaq	(%rcx,%r13,8), %rdi
	leaq	(%r13,%r11), %r13
	xorl	%ecx, %ecx
	.p2align	4, 0x90
.LBB1_5:
	vmovdqu	-96(%rdi,%rcx,8), %ymm6
	vmovdqu	-64(%rdi,%rcx,8), %ymm7
	vmovdqu	-32(%rdi,%rcx,8), %ymm5
	vmovdqu	(%rdi,%rcx,8), %ymm4
	vmovq	%xmm6, %rbp
	vmovss	(%rbx,%rbp,4), %xmm0
	vpextrq	$1, %xmm6, %rbp
	vextracti128	$1, %ymm6, %xmm6
	vinsertps	$16, (%rbx,%rbp,4), %xmm0, %xmm0
	vmovq	%xmm6, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm0, %xmm0
	vpextrq	$1, %xmm6, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm0, %xmm0
	vmovq	%xmm7, %rbp
	vmovss	(%rbx,%rbp,4), %xmm6
	vpextrq	$1, %xmm7, %rbp
	vextracti128	$1, %ymm7, %xmm7
	vmulps	-48(%rdx,%rcx,4), %xmm0, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm6, %xmm6
	vmovq	%xmm7, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm6, %xmm6
	vpextrq	$1, %xmm7, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm6, %xmm6
	vmovq	%xmm5, %rbp
	vaddps	%xmm0, %xmm8, %xmm8
	vmovss	(%rbx,%rbp,4), %xmm7
	vpextrq	$1, %xmm5, %rbp
	vextracti128	$1, %ymm5, %xmm5
	vmulps	-32(%rdx,%rcx,4), %xmm6, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm7, %xmm7
	vmovq	%xmm5, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm5, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm7, %xmm5
	vmovq	%xmm4, %rbp
	vaddps	%xmm0, %xmm1, %xmm1
	vmovss	(%rbx,%rbp,4), %xmm7
	vpextrq	$1, %xmm4, %rbp
	vextracti128	$1, %ymm4, %xmm4
	vmulps	-16(%rdx,%rcx,4), %xmm5, %xmm0
	vinsertps	$16, (%rbx,%rbp,4), %xmm7, %xmm7
	vmovq	%xmm4, %rbp
	vinsertps	$32, (%rbx,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm4, %rbp
	vinsertps	$48, (%rbx,%rbp,4), %xmm7, %xmm4
	vaddps	%xmm0, %xmm2, %xmm2
	vmulps	(%rdx,%rcx,4), %xmm4, %xmm4
	addq	$16, %rcx
	cmpq	%rcx, %r11
	vaddps	%xmm4, %xmm3, %xmm3
	jne	.LBB1_5
	vaddps	%xmm8, %xmm1, %xmm0
	movq	(%rsp), %rdx
	movq	heartbeat@GOTPCREL(%rip), %rdi
	cmpq	%r11, %r9
	vaddps	%xmm0, %xmm2, %xmm0
	vaddps	%xmm0, %xmm3, %xmm0
	vpermilpd	$1, %xmm0, %xmm1
	vaddps	%xmm1, %xmm0, %xmm0
	vhaddps	%xmm0, %xmm0, %xmm0
	je	.LBB1_8
	.p2align	4, 0x90
.LBB1_7:
	movq	(%r15,%r13,8), %rcx
	vmovss	(%r12,%r13,4), %xmm1
	incq	%r13
	cmpq	%rax, %r13
	vmulss	(%rbx,%rcx,4), %xmm1, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	jb	.LBB1_7
.LBB1_8:
	movq	%rax, %r13
.LBB1_9:
	cmpq	%r14, %r13
	jae	.LBB1_12
	cmpl	$0, (%rdi)
	je	.LBB1_2
	movq	%r12, %rdi
	movq	%r15, %rdx
	movq	%rbx, %rcx
	movq	%r8, 8(%rsp)
	movq	%r13, %r9
	pushq	%r10
	.cfi_adjust_cfa_offset 8
	pushq	%r14
	.cfi_adjust_cfa_offset 8
	movq	%rsi, %rbp
	vmovaps	%xmm0, 48(%rsp)
	vzeroupper
	callq	_Z25col_loop_handler_col_loopPfPmS0_S_S_mmfS_@PLT
	vmovaps	48(%rsp), %xmm0
	movq	heartbeat@GOTPCREL(%rip), %rdi
	movq	16(%rsp), %rdx
	movq	24(%rsp), %r8
	movq	136(%rsp), %r10
	vxorps	%xmm9, %xmm9, %xmm9
	movq	%rbp, %rsi
	addq	$16, %rsp
	.cfi_adjust_cfa_offset -16
	testl	%eax, %eax
	je	.LBB1_2
	jmp	.LBB1_13
.LBB1_12:
	vmovss	%xmm0, (%r10)
.LBB1_13:
	addq	$56, %rsp
	.cfi_def_cfa_offset 56
	popq	%rbx
	.cfi_def_cfa_offset 48
	popq	%r12
	.cfi_def_cfa_offset 40
	popq	%r13
	.cfi_def_cfa_offset 32
	popq	%r14
	.cfi_def_cfa_offset 24
	popq	%r15
	.cfi_def_cfa_offset 16
	popq	%rbp
	.cfi_def_cfa_offset 8
	vzeroupper
	retq
.Lfunc_end1:
	.size	_Z23spmv_interrupt_col_loopPfPmS0_S_S_mmfS_, .Lfunc_end1-_Z23spmv_interrupt_col_loopPfPmS0_S_S_mmfS_
	.cfi_endproc


	.ident	"clang version 7.1.0 (tags/RELEASE_710/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym heartbeat
