	.text
	.file	"srad.tpal_orig.cpp"
	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2
.LCPI0_0:
	.long	1065353216
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3
.LCPI0_1:
	.quad	4602678819172646912
.LCPI0_2:
	.quad	4589168020290535424
.LCPI0_3:
	.quad	4598175219545276416
.LCPI0_4:
	.quad	4607182418800017408
.LCPI0_5:
	.quad	-4634204016564240384
	.text
	.globl	_Z9srad_tpaliiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.p2align	4, 0x90
	.type	_Z9srad_tpaliiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f,@function
_Z9srad_tpaliiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f:
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
	subq	$232, %rsp
	.cfi_def_cfa_offset 288
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	testl	%edi, %edi
	vmovss	%xmm1, 44(%rsp)
	movq	%r8, 104(%rsp)
	movq	%rcx, 96(%rsp)
	movq	%rdx, 88(%rsp)
	movl	%edi, 20(%rsp)
	jle	.LBB0_47
	vmovaps	%xmm0, %xmm10
	vaddss	.LCPI0_0(%rip), %xmm0, %xmm0
	movq	344(%rsp), %r10
	movq	320(%rsp), %r14
	movq	288(%rsp), %rdx
	vmovsd	.LCPI0_1(%rip), %xmm14
	vmovsd	.LCPI0_5(%rip), %xmm15
	vmovsd	.LCPI0_3(%rip), %xmm7
	vmovsd	.LCPI0_4(%rip), %xmm8
	movslq	20(%rsp), %rax
	movq	%r9, %rbx
	xorl	%r13d, %r13d
	movl	%esi, 16(%rsp)
	vbroadcastss	%xmm10, %ymm12
	movq	%rax, 112(%rsp)
	movl	%esi, %eax
	movq	%rax, 32(%rsp)
	xorl	%eax, %eax
	movq	%rax, 24(%rsp)
	vmulss	%xmm10, %xmm0, %xmm11
	vbroadcastss	%xmm11, %ymm13
	.p2align	4, 0x90
.LBB0_2:
	testl	%esi, %esi
	jle	.LBB0_28
	movq	24(%rsp), %rax
	xorl	%r12d, %r12d
	movl	%eax, %r15d
	movl	%eax, %ecx
	imull	%esi, %r15d
	imull	%esi, %ecx
	vmovd	%r15d, %xmm0
	movq	%rcx, 120(%rsp)
	vpbroadcastd	%xmm0, %ymm0
	vmovdqu	%ymm0, 192(%rsp)
	.p2align	4, 0x90
.LBB0_4:
	leal	64(%r12), %ebp
	cmpl	%esi, %ebp
	cmovgl	%esi, %ebp
	cmpl	%ebp, %r12d
	jge	.LBB0_13
	movq	328(%rsp), %rax
	movq	24(%rsp), %rcx
	movslq	%ebp, %r11
	movslq	%r12d, %r8
	movl	%ebp, 48(%rsp)
	movl	(%rax,%rcx,4), %r9d
	movq	336(%rsp), %rax
	movl	(%rax,%rcx,4), %edi
	movq	%r11, %rax
	imull	%esi, %r9d
	subq	%r8, %rax
	imull	%esi, %edi
	cmpq	$8, %rax
	jb	.LBB0_6
	movq	120(%rsp), %rcx
	movq	%rax, 64(%rsp)
	movq	%r8, %rax
	notq	%rax
	addq	%r11, %rax
	leal	(%rcx,%r12), %edx
	leal	(%rdx,%rax), %ecx
	cmpl	%edx, %ecx
	jl	.LBB0_6
	movq	%rax, %rsi
	shrq	$32, %rsi
	jne	.LBB0_26
	leal	(%r12,%r9), %edx
	leal	(%rdx,%rax), %ecx
	cmpl	%edx, %ecx
	jl	.LBB0_26
	testq	%rsi, %rsi
	jne	.LBB0_26
	movq	%r9, 160(%rsp)
	leal	(%r12,%rdi), %r9d
	movq	%rdi, 128(%rsp)
	leal	(%r9,%rax), %eax
	cmpl	%r9d, %eax
	jl	.LBB0_25
	testq	%rsi, %rsi
	jne	.LBB0_25
	movq	64(%rsp), %rsi
	movq	352(%rsp), %rax
	vmovdqu	192(%rsp), %ymm9
	movq	%rdx, %rcx
	movl	%r9d, %ebp
	addl	%r13d, %r12d
	xorl	%r9d, %r9d
	movl	%ecx, %ecx
	andq	$-8, %rsi
	leaq	(%rax,%r8,4), %rax
	leaq	(%rsi,%r8), %rdx
	leaq	(%r10,%r8,4), %r8
	.p2align	4, 0x90
.LBB0_23:
	vpaddd	(%rax,%r9,4), %ymm9, %ymm2
	leal	(%rcx,%r9), %edi
	leal	(%r12,%r9), %r10d
	movslq	%edi, %rdi
	movslq	%r10d, %r10
	vmovups	(%rbx,%rdi,4), %ymm8
	leal	(%rbp,%r9), %edi
	movslq	%edi, %rdi
	vmovups	(%rbx,%rdi,4), %ymm1
	vextracti128	$1, %ymm2, %xmm3
	vpmovsxdq	%xmm2, %ymm2
	vmovq	%xmm2, %rdi
	vextracti128	$1, %ymm2, %xmm6
	vpmovsxdq	%xmm3, %ymm3
	vmovss	(%rbx,%rdi,4), %xmm4
	vmovq	%xmm3, %rdi
	vmovss	(%rbx,%rdi,4), %xmm5
	vpextrq	$1, %xmm3, %rdi
	vinsertps	$16, (%rbx,%rdi,4), %xmm5, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm3, %xmm2
	vinsertps	$16, (%rbx,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%rbx,%rdi,4), %xmm5, %xmm3
	vpaddd	(%r8,%r9,4), %ymm9, %ymm5
	vmovq	%xmm6, %rdi
	addq	$8, %r9
	cmpq	%r9, %rsi
	vinsertps	$32, (%rbx,%rdi,4), %xmm4, %xmm4
	vpmovsxdq	%xmm5, %ymm7
	vextracti128	$1, %ymm5, %xmm5
	vmovq	%xmm7, %rdi
	vpmovsxdq	%xmm5, %ymm5
	vmovss	(%rbx,%rdi,4), %xmm0
	vpextrq	$1, %xmm6, %rdi
	vinsertps	$48, (%rbx,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm5, %rdi
	vmovss	(%rbx,%rdi,4), %xmm6
	vpextrq	$1, %xmm5, %rdi
	vextracti128	$1, %ymm5, %xmm5
	vinsertps	$16, (%rbx,%rdi,4), %xmm6, %xmm6
	vmovq	%xmm5, %rdi
	vinsertps	$32, (%rbx,%rdi,4), %xmm6, %xmm6
	vpextrq	$1, %xmm7, %rdi
	vinsertps	$16, (%rbx,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm7, %xmm2
	vinsertps	$48, (%rbx,%rdi,4), %xmm3, %xmm3
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%rbx,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm5, %rdi
	vinsertps	$48, (%rbx,%rdi,4), %xmm6, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vmovups	(%rbx,%r10,4), %ymm2
	vinsertps	$48, (%rbx,%rdi,4), %xmm0, %xmm0
	movq	288(%rsp), %rdi
	vinsertf128	$1, %xmm3, %ymm4, %ymm3
	vsubps	%ymm2, %ymm8, %ymm6
	vsubps	%ymm2, %ymm1, %ymm1
	vsubps	%ymm2, %ymm3, %ymm3
	vinsertf128	$1, %xmm5, %ymm0, %ymm0
	vmovups	%ymm6, (%rdi,%r10,4)
	movq	296(%rsp), %rdi
	vmulps	%ymm6, %ymm6, %ymm4
	vmulps	%ymm1, %ymm1, %ymm5
	vsubps	%ymm2, %ymm0, %ymm0
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm3, %ymm3, %ymm5
	vmovups	%ymm1, (%rdi,%r10,4)
	movq	304(%rsp), %rdi
	vaddps	%ymm1, %ymm6, %ymm1
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm0, %ymm0, %ymm5
	vmulps	%ymm2, %ymm2, %ymm6
	vaddps	%ymm3, %ymm1, %ymm1
	vaddps	%ymm5, %ymm4, %ymm4
	vmovups	%ymm3, (%rdi,%r10,4)
	movq	312(%rsp), %rdi
	vbroadcastsd	.LCPI0_1(%rip), %ymm3
	vmovups	%ymm0, (%rdi,%r10,4)
	vaddps	%ymm0, %ymm1, %ymm0
	vdivps	%ymm6, %ymm4, %ymm1
	vbroadcastsd	.LCPI0_2(%rip), %ymm6
	vdivps	%ymm2, %ymm0, %ymm0
	vextractf128	$1, %ymm1, %xmm2
	vcvtps2pd	%xmm1, %ymm1
	vcvtps2pd	%xmm2, %ymm2
	vmulps	%ymm0, %ymm0, %ymm4
	vmulpd	%ymm3, %ymm1, %ymm1
	vmulpd	%ymm3, %ymm2, %ymm2
	vextractf128	$1, %ymm4, %xmm5
	vcvtps2pd	%xmm4, %ymm4
	vcvtps2pd	%xmm5, %ymm5
	vmulpd	%ymm6, %ymm4, %ymm4
	vmulpd	%ymm6, %ymm5, %ymm3
	vsubpd	%ymm4, %ymm1, %ymm1
	vextractf128	$1, %ymm0, %xmm4
	vcvtps2pd	%xmm0, %ymm0
	vsubpd	%ymm3, %ymm2, %ymm2
	vcvtps2pd	%xmm4, %ymm3
	vbroadcastsd	.LCPI0_3(%rip), %ymm4
	vcvtpd2ps	%ymm1, %xmm1
	vcvtpd2ps	%ymm2, %xmm2
	vmulpd	%ymm4, %ymm0, %ymm0
	vmulpd	%ymm4, %ymm3, %ymm3
	vbroadcastsd	.LCPI0_4(%rip), %ymm4
	vinsertf128	$1, %xmm2, %ymm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm3, %ymm3
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm3, %xmm2
	vinsertf128	$1, %xmm2, %ymm0, %ymm0
	vbroadcastss	.LCPI0_0(%rip), %ymm2
	vmulps	%ymm0, %ymm0, %ymm0
	vdivps	%ymm0, %ymm1, %ymm0
	vsubps	%ymm12, %ymm0, %ymm0
	vdivps	%ymm13, %ymm0, %ymm0
	vextractf128	$1, %ymm0, %xmm1
	vcvtps2pd	%xmm0, %ymm0
	vcvtps2pd	%xmm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm1, %ymm1
	vdivpd	%ymm0, %ymm4, %ymm0
	vdivpd	%ymm1, %ymm4, %ymm1
	vxorpd	%xmm4, %xmm4, %xmm4
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm1, %xmm1
	vinsertf128	$1, %xmm1, %ymm0, %ymm0
	vmovupd	%ymm0, (%r14,%r10,4)
	vcmpltps	%ymm4, %ymm0, %ymm1
	vcmpnltps	%ymm4, %ymm0, %ymm3
	vcmpltps	%ymm0, %ymm2, %ymm0
	vandps	%ymm3, %ymm0, %ymm0
	vmaskmovps	%ymm2, %ymm0, (%r14,%r10,4)
	vmaskmovps	%ymm4, %ymm1, (%r14,%r10,4)
	jne	.LBB0_23
	cmpq	%rsi, 64(%rsp)
	movl	16(%rsp), %esi
	movq	344(%rsp), %r10
	vmovsd	.LCPI0_3(%rip), %xmm7
	vmovsd	.LCPI0_4(%rip), %xmm8
	movq	160(%rsp), %r9
	movq	128(%rsp), %rdi
	jne	.LBB0_7
	jmp	.LBB0_12
.LBB0_25:
	movq	160(%rsp), %r9
	movq	128(%rsp), %rdi
.LBB0_26:
	movl	16(%rsp), %esi
.LBB0_6:
	movq	%r8, %rdx
.LBB0_7:
	movl	%r9d, %ecx
	movq	312(%rsp), %r8
	movq	304(%rsp), %r9
	movq	296(%rsp), %r12
	movl	%edi, %eax
	.p2align	4, 0x90
.LBB0_8:
	leal	(%r13,%rdx), %edi
	leal	(%rcx,%rdx), %ebp
	movslq	%edi, %rdi
	movslq	%ebp, %rbp
	vmovss	(%rbx,%rdi,4), %xmm0
	vmovss	(%rbx,%rbp,4), %xmm1
	movq	288(%rsp), %rbp
	vsubss	%xmm0, %xmm1, %xmm1
	vmulss	%xmm0, %xmm0, %xmm9
	vmovss	%xmm1, (%rbp,%rdi,4)
	leal	(%rax,%rdx), %ebp
	vmulss	%xmm1, %xmm1, %xmm5
	movslq	%ebp, %rbp
	vmovss	(%rbx,%rbp,4), %xmm2
	movq	352(%rsp), %rbp
	movl	(%rbp,%rdx,4), %ebp
	vsubss	%xmm0, %xmm2, %xmm2
	vmulss	%xmm2, %xmm2, %xmm6
	vaddss	%xmm2, %xmm1, %xmm1
	vmovss	%xmm2, (%r12,%rdi,4)
	vaddss	%xmm6, %xmm5, %xmm5
	addl	%r15d, %ebp
	movslq	%ebp, %rbp
	vmovss	(%rbx,%rbp,4), %xmm3
	movl	(%r10,%rdx,4), %ebp
	addl	%r15d, %ebp
	vsubss	%xmm0, %xmm3, %xmm3
	movslq	%ebp, %rbp
	vmovss	(%rbx,%rbp,4), %xmm4
	vmulss	%xmm3, %xmm3, %xmm6
	vaddss	%xmm3, %xmm1, %xmm1
	vmovss	%xmm3, (%r9,%rdi,4)
	vaddss	%xmm6, %xmm5, %xmm5
	vsubss	%xmm0, %xmm4, %xmm4
	vmulss	%xmm4, %xmm4, %xmm6
	vaddss	%xmm4, %xmm1, %xmm1
	vmovss	%xmm4, (%r8,%rdi,4)
	vaddss	%xmm6, %xmm5, %xmm5
	vdivss	%xmm0, %xmm1, %xmm0
	vdivss	%xmm9, %xmm5, %xmm5
	vmulss	%xmm0, %xmm0, %xmm2
	vcvtss2sd	%xmm5, %xmm5, %xmm1
	vcvtss2sd	%xmm2, %xmm2, %xmm2
	vmulsd	%xmm14, %xmm1, %xmm1
	vmulsd	%xmm15, %xmm2, %xmm2
	vaddsd	%xmm2, %xmm1, %xmm1
	vcvtsd2ss	%xmm1, %xmm1, %xmm1
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	%xmm7, %xmm0, %xmm0
	vaddsd	%xmm8, %xmm0, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm0
	vmulss	%xmm0, %xmm0, %xmm0
	vdivss	%xmm0, %xmm1, %xmm0
	vsubss	%xmm10, %xmm0, %xmm0
	vdivss	%xmm11, %xmm0, %xmm0
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vaddsd	%xmm8, %xmm0, %xmm0
	vdivsd	%xmm0, %xmm8, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm1
	vxorpd	%xmm0, %xmm0, %xmm0
	vucomiss	%xmm1, %xmm0
	vmovss	%xmm1, (%r14,%rdi,4)
	ja	.LBB0_10
	vmovss	.LCPI0_0(%rip), %xmm0
	vucomiss	%xmm0, %xmm1
	jbe	.LBB0_11
.LBB0_10:
	vmovss	%xmm0, (%r14,%rdi,4)
.LBB0_11:
	incq	%rdx
	cmpq	%r11, %rdx
	jl	.LBB0_8
.LBB0_12:
	movl	48(%rsp), %eax
	movq	288(%rsp), %rbp
	movl	%eax, %r12d
	movq	%rbp, %rdx
.LBB0_13:
	cmpl	%esi, %r12d
	jge	.LBB0_28
	movq	heartbeat@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	je	.LBB0_4
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movl	28(%rsp), %edi
	movq	%rdx, %rbp
	movq	32(%rsp), %rdx
	vmovss	52(%rsp), %xmm1
	movl	%r12d, %r8d
	movl	%esi, %r9d
	vmovaps	%xmm10, %xmm0
	movl	%edi, %ecx
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r10
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r14
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%rbp
	.cfi_adjust_cfa_offset 8
	pushq	%rbx
	.cfi_adjust_cfa_offset 8
	pushq	192(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	192(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	192(%rsp)
	.cfi_adjust_cfa_offset 8
	vmovaps	%xmm10, 160(%rsp)
	vmovaps	%xmm11, 176(%rsp)
	vmovups	%ymm12, 272(%rsp)
	vmovups	%ymm13, 240(%rsp)
	vzeroupper
	callq	_Z12srad_handleriiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f@PLT
	vmovsd	.LCPI0_4(%rip), %xmm8
	vmovsd	.LCPI0_3(%rip), %xmm7
	vmovsd	.LCPI0_5(%rip), %xmm15
	vmovsd	.LCPI0_1(%rip), %xmm14
	vmovups	240(%rsp), %ymm13
	vmovups	272(%rsp), %ymm12
	vmovaps	176(%rsp), %xmm11
	vmovaps	160(%rsp), %xmm10
	movq	456(%rsp), %r10
	movl	128(%rsp), %esi
	movq	%rbp, %rdx
	addq	$112, %rsp
	.cfi_adjust_cfa_offset -112
	testl	%eax, %eax
	je	.LBB0_4
	jmp	.LBB0_47
	.p2align	4, 0x90
.LBB0_28:
	movq	24(%rsp), %rax
	addq	32(%rsp), %r13
	movq	%rax, %rcx
	incq	%rcx
	cmpq	112(%rsp), %rcx
	movq	%rcx, %rax
	movq	%rcx, 24(%rsp)
	jl	.LBB0_2
	cmpl	$0, 20(%rsp)
	jle	.LBB0_47
	testl	%esi, %esi
	jle	.LBB0_47
	vmovss	44(%rsp), %xmm0
	movl	20(%rsp), %eax
	xorl	%r13d, %r13d
	xorl	%r15d, %r15d
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	.LCPI0_3(%rip), %xmm0, %xmm0
	movq	%rax, 24(%rsp)
	movq	32(%rsp), %rax
	movl	%eax, %r11d
	leaq	-1(%rax), %rcx
	andl	$-8, %r11d
	movq	%rcx, 64(%rsp)
	vbroadcastsd	%xmm0, %ymm1
	jmp	.LBB0_41
	.p2align	4, 0x90
.LBB0_32:
	movq	64(%rsp), %rsi
	movq	32(%rsp), %r10
	leal	(%r12,%rsi), %eax
	cmpl	%r12d, %eax
	jl	.LBB0_40
	movq	%rcx, 48(%rsp)
	movq	%rsi, %rcx
	shrq	$32, %rcx
	jne	.LBB0_39
	movq	48(%rsp), %rdi
	leal	(%rdi,%rsi), %eax
	cmpl	%edi, %eax
	jl	.LBB0_39
	testq	%rcx, %rcx
	movq	344(%rsp), %rsi
	movq	312(%rsp), %rdi
	movq	304(%rsp), %r8
	movq	296(%rsp), %r9
	movq	48(%rsp), %rcx
	movl	$0, %eax
	jne	.LBB0_44
	vmovd	%r12d, %xmm2
	movl	%ecx, %ecx
	xorl	%eax, %eax
	movq	%rdx, %r10
	vpbroadcastd	%xmm2, %ymm2
	.p2align	4, 0x90
.LBB0_37:
	vpaddd	(%rsi,%rax,4), %ymm2, %ymm4
	leal	(%rcx,%rax), %ebp
	leal	(%r13,%rax), %edx
	addq	$8, %rax
	movslq	%ebp, %rbp
	movslq	%edx, %rdx
	cmpq	%rax, %r11
	vmovups	(%r14,%rbp,4), %ymm3
	vmulps	(%r9,%rdx,4), %ymm3, %ymm3
	vextracti128	$1, %ymm4, %xmm5
	vpmovsxdq	%xmm4, %ymm4
	vmovq	%xmm4, %rbp
	vpmovsxdq	%xmm5, %ymm5
	vmovss	(%r14,%rbp,4), %xmm6
	vmovq	%xmm5, %rbp
	vmovss	(%r14,%rbp,4), %xmm7
	vpextrq	$1, %xmm5, %rbp
	vextracti128	$1, %ymm5, %xmm5
	vinsertps	$16, (%r14,%rbp,4), %xmm7, %xmm7
	vmovq	%xmm5, %rbp
	vinsertps	$32, (%r14,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm5, %rbp
	vmovups	(%r14,%rdx,4), %ymm5
	vinsertps	$48, (%r14,%rbp,4), %xmm7, %xmm7
	vpextrq	$1, %xmm4, %rbp
	vextracti128	$1, %ymm4, %xmm4
	vinsertps	$16, (%r14,%rbp,4), %xmm6, %xmm6
	vmovq	%xmm4, %rbp
	vinsertps	$32, (%r14,%rbp,4), %xmm6, %xmm6
	vpextrq	$1, %xmm4, %rbp
	vmulps	(%r10,%rdx,4), %ymm5, %ymm4
	vinsertps	$48, (%r14,%rbp,4), %xmm6, %xmm6
	vaddps	%ymm3, %ymm4, %ymm3
	vmulps	(%r8,%rdx,4), %ymm5, %ymm4
	vcvtps2pd	(%rbx,%rdx,4), %ymm5
	vaddps	%ymm4, %ymm3, %ymm3
	vinsertf128	$1, %xmm7, %ymm6, %ymm4
	vmulps	(%rdi,%rdx,4), %ymm4, %ymm4
	vaddps	%ymm4, %ymm3, %ymm3
	vcvtps2pd	16(%rbx,%rdx,4), %ymm4
	vextractf128	$1, %ymm3, %xmm6
	vcvtps2pd	%xmm3, %ymm3
	vcvtps2pd	%xmm6, %ymm6
	vmulpd	%ymm3, %ymm1, %ymm3
	vmulpd	%ymm6, %ymm1, %ymm6
	vaddpd	%ymm5, %ymm3, %ymm3
	vaddpd	%ymm4, %ymm6, %ymm4
	vcvtpd2ps	%ymm3, %xmm3
	vcvtpd2ps	%ymm4, %xmm4
	vinsertf128	$1, %xmm4, %ymm3, %ymm3
	vmovupd	%ymm3, (%rbx,%rdx,4)
	jne	.LBB0_37
	movq	32(%rsp), %rcx
	movq	%r10, %rbp
	movq	%r11, %rax
	cmpq	%rcx, %r11
	movq	%rcx, %r10
	movq	48(%rsp), %rcx
	jne	.LBB0_44
	jmp	.LBB0_46
.LBB0_39:
	movq	344(%rsp), %rsi
	movq	312(%rsp), %rdi
	movq	304(%rsp), %r8
	movq	296(%rsp), %r9
	movq	48(%rsp), %rcx
	jmp	.LBB0_43
.LBB0_40:
	movq	344(%rsp), %rsi
	movq	312(%rsp), %rdi
	movq	304(%rsp), %r8
	movq	296(%rsp), %r9
	jmp	.LBB0_43
	.p2align	4, 0x90
.LBB0_41:
	movq	336(%rsp), %rax
	movl	%r15d, %r12d
	imull	%esi, %r12d
	movl	(%rax,%r15,4), %ecx
	imull	%esi, %ecx
	cmpl	$8, %esi
	jae	.LBB0_32
	movq	344(%rsp), %rsi
	movq	312(%rsp), %rdi
	movq	304(%rsp), %r8
	movq	296(%rsp), %r9
	movq	32(%rsp), %r10
.LBB0_43:
	xorl	%eax, %eax
.LBB0_44:
	movl	%ecx, %ecx
	.p2align	4, 0x90
.LBB0_45:
	leal	(%rcx,%rax), %ebp
	leal	(%r13,%rax), %edx
	movslq	%ebp, %rbp
	movslq	%edx, %rdx
	vmovss	(%r14,%rbp,4), %xmm3
	movl	(%rsi,%rax,4), %ebp
	vmovss	(%r14,%rdx,4), %xmm2
	incq	%rax
	vmulss	(%r9,%rdx,4), %xmm3, %xmm3
	addl	%r12d, %ebp
	cmpq	%rax, %r10
	movslq	%ebp, %rbp
	vmovss	(%r14,%rbp,4), %xmm4
	movq	288(%rsp), %rbp
	vmulss	(%rbp,%rdx,4), %xmm2, %xmm5
	vmulss	(%r8,%rdx,4), %xmm2, %xmm2
	vaddss	%xmm3, %xmm5, %xmm3
	vaddss	%xmm2, %xmm3, %xmm2
	vmulss	(%rdi,%rdx,4), %xmm4, %xmm3
	vaddss	%xmm3, %xmm2, %xmm2
	vmovss	(%rbx,%rdx,4), %xmm3
	vcvtss2sd	%xmm2, %xmm2, %xmm2
	vmulsd	%xmm2, %xmm0, %xmm2
	vcvtss2sd	%xmm3, %xmm3, %xmm3
	vaddsd	%xmm3, %xmm2, %xmm2
	vcvtsd2ss	%xmm2, %xmm2, %xmm2
	vmovd	%xmm2, (%rbx,%rdx,4)
	jne	.LBB0_45
.LBB0_46:
	movl	16(%rsp), %esi
	incq	%r15
	addq	%r10, %r13
	cmpq	24(%rsp), %r15
	movq	%rbp, %rdx
	jne	.LBB0_41
.LBB0_47:
	addq	$232, %rsp
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
	.size	_Z9srad_tpaliiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f, .Lfunc_end0-_Z9srad_tpaliiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.cfi_endproc

	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2
.LCPI1_0:
	.long	1065353216
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3
.LCPI1_1:
	.quad	4602678819172646912
.LCPI1_2:
	.quad	4589168020290535424
.LCPI1_3:
	.quad	4598175219545276416
.LCPI1_4:
	.quad	4607182418800017408
.LCPI1_5:
	.quad	-4634204016564240384
	.text
	.globl	_Z11srad_tpal_1iiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.p2align	4, 0x90
	.type	_Z11srad_tpal_1iiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f,@function
_Z11srad_tpal_1iiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f:
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
	subq	$264, %rsp
	.cfi_def_cfa_offset 320
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpl	%ecx, %edx
	vmovss	%xmm1, 52(%rsp)
	movq	%r9, 160(%rsp)
	movq	%r8, 152(%rsp)
	movl	%edi, 48(%rsp)
	movl	%ecx, 32(%rsp)
	movq	%rdx, %rax
	movq	%rdx, 56(%rsp)
	jge	.LBB1_30
	vmovaps	%xmm0, %xmm10
	vaddss	.LCPI1_0(%rip), %xmm0, %xmm0
	movq	56(%rsp), %rax
	movq	400(%rsp), %r10
	movq	392(%rsp), %r11
	movq	384(%rsp), %r9
	movq	376(%rsp), %r8
	movq	368(%rsp), %r15
	movq	360(%rsp), %rbp
	movq	328(%rsp), %r14
	vmovsd	.LCPI1_1(%rip), %xmm14
	vmovsd	.LCPI1_5(%rip), %xmm15
	vmovsd	.LCPI1_3(%rip), %xmm7
	vmovsd	.LCPI1_4(%rip), %xmm8
	movl	%esi, 36(%rsp)
	vbroadcastss	%xmm10, %ymm12
	movslq	%eax, %rcx
	movl	%eax, %r12d
	movl	%esi, %eax
	movq	%rcx, 24(%rsp)
	movslq	32(%rsp), %rcx
	imull	%esi, %r12d
	movq	%rax, 168(%rsp)
	xorl	%eax, %eax
	vmulss	%xmm10, %xmm0, %xmm11
	movq	%rax, 40(%rsp)
	vbroadcastss	%xmm11, %ymm13
	movq	%rcx, 176(%rsp)
	.p2align	4, 0x90
.LBB1_2:
	testl	%esi, %esi
	jle	.LBB1_29
	movq	56(%rsp), %rax
	movq	40(%rsp), %rcx
	xorl	%ebx, %ebx
	leal	(%rcx,%rax), %eax
	imull	%esi, %eax
	movq	%rax, 184(%rsp)
	movq	24(%rsp), %rax
	movl	%eax, %r13d
	imull	%esi, %r13d
	vmovd	%r13d, %xmm0
	vpbroadcastd	%xmm0, %ymm0
	vmovdqu	%ymm0, 224(%rsp)
	.p2align	4, 0x90
.LBB1_4:
	leal	64(%rbx), %ecx
	cmpl	%esi, %ecx
	cmovgl	%esi, %ecx
	cmpl	%ecx, %ebx
	jge	.LBB1_13
	movq	24(%rsp), %rax
	movl	%ecx, 64(%rsp)
	movslq	%ecx, %rcx
	movslq	%ebx, %rdi
	movl	(%r8,%rax,4), %edx
	movl	(%r9,%rax,4), %ebp
	movq	%rcx, %rax
	subq	%rdi, %rax
	imull	%esi, %edx
	imull	%esi, %ebp
	cmpq	$8, %rax
	movq	%rdx, 80(%rsp)
	jae	.LBB1_16
.LBB1_6:
	movq	%rdi, %rdx
.LBB1_7:
	movl	80(%rsp), %ebx
	movl	%ebp, %eax
	.p2align	4, 0x90
.LBB1_8:
	leal	(%r12,%rdx), %edi
	leal	(%rbx,%rdx), %ebp
	movslq	%edi, %rdi
	movslq	%ebp, %rbp
	vmovss	(%r14,%rdi,4), %xmm0
	vmovss	(%r14,%rbp,4), %xmm1
	movq	336(%rsp), %rbp
	vsubss	%xmm0, %xmm1, %xmm1
	vmulss	%xmm0, %xmm0, %xmm9
	vmovss	%xmm1, (%rbp,%rdi,4)
	leal	(%rax,%rdx), %ebp
	vmulss	%xmm1, %xmm1, %xmm5
	movslq	%ebp, %rbp
	vmovss	(%r14,%rbp,4), %xmm2
	movq	344(%rsp), %rbp
	vsubss	%xmm0, %xmm2, %xmm2
	vmovss	%xmm2, (%rbp,%rdi,4)
	movl	(%r10,%rdx,4), %ebp
	vmulss	%xmm2, %xmm2, %xmm6
	vaddss	%xmm2, %xmm1, %xmm1
	vaddss	%xmm6, %xmm5, %xmm5
	addl	%r13d, %ebp
	movslq	%ebp, %rbp
	vmovss	(%r14,%rbp,4), %xmm3
	movq	352(%rsp), %rbp
	vsubss	%xmm0, %xmm3, %xmm3
	vmovss	%xmm3, (%rbp,%rdi,4)
	movl	(%r11,%rdx,4), %ebp
	vmulss	%xmm3, %xmm3, %xmm6
	vaddss	%xmm3, %xmm1, %xmm1
	vaddss	%xmm6, %xmm5, %xmm5
	addl	%r13d, %ebp
	movslq	%ebp, %rbp
	vmovss	(%r14,%rbp,4), %xmm4
	movq	360(%rsp), %rbp
	vsubss	%xmm0, %xmm4, %xmm4
	vmulss	%xmm4, %xmm4, %xmm6
	vaddss	%xmm4, %xmm1, %xmm1
	vmovss	%xmm4, (%rbp,%rdi,4)
	vaddss	%xmm6, %xmm5, %xmm5
	vdivss	%xmm0, %xmm1, %xmm0
	vdivss	%xmm9, %xmm5, %xmm5
	vmulss	%xmm0, %xmm0, %xmm2
	vcvtss2sd	%xmm5, %xmm5, %xmm1
	vcvtss2sd	%xmm2, %xmm2, %xmm2
	vmulsd	%xmm14, %xmm1, %xmm1
	vmulsd	%xmm15, %xmm2, %xmm2
	vaddsd	%xmm2, %xmm1, %xmm1
	vcvtsd2ss	%xmm1, %xmm1, %xmm1
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	%xmm7, %xmm0, %xmm0
	vaddsd	%xmm8, %xmm0, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm0
	vmulss	%xmm0, %xmm0, %xmm0
	vdivss	%xmm0, %xmm1, %xmm0
	vsubss	%xmm10, %xmm0, %xmm0
	vdivss	%xmm11, %xmm0, %xmm0
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vaddsd	%xmm8, %xmm0, %xmm0
	vdivsd	%xmm0, %xmm8, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm1
	vxorpd	%xmm0, %xmm0, %xmm0
	vucomiss	%xmm1, %xmm0
	vmovss	%xmm1, (%r15,%rdi,4)
	ja	.LBB1_10
	vmovss	.LCPI1_0(%rip), %xmm0
	vucomiss	%xmm0, %xmm1
	jbe	.LBB1_11
.LBB1_10:
	vmovss	%xmm0, (%r15,%rdi,4)
.LBB1_11:
	incq	%rdx
	cmpq	%rcx, %rdx
	jl	.LBB1_8
.LBB1_12:
	movl	64(%rsp), %eax
	movq	360(%rsp), %rbp
	movl	%eax, %ebx
.LBB1_13:
	cmpl	%esi, %ebx
	jge	.LBB1_29
	movq	heartbeat@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	je	.LBB1_4
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movq	32(%rsp), %rdx
	movl	56(%rsp), %edi
	movl	40(%rsp), %ecx
	vmovss	60(%rsp), %xmm1
	movl	%ebx, %r8d
	movl	%esi, %r9d
	vmovaps	%xmm10, %xmm0
	pushq	%r10
	.cfi_adjust_cfa_offset 8
	pushq	%r11
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r15
	.cfi_adjust_cfa_offset 8
	pushq	%rbp
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r14
	.cfi_adjust_cfa_offset 8
	pushq	408(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	256(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	256(%rsp)
	.cfi_adjust_cfa_offset 8
	vmovaps	%xmm10, 192(%rsp)
	movq	%r10, %rbp
	vmovaps	%xmm11, 176(%rsp)
	vmovups	%ymm12, 208(%rsp)
	vmovups	%ymm13, 304(%rsp)
	vzeroupper
	callq	_Z14srad_handler_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f@PLT
	movq	%rbp, %r10
	vmovsd	.LCPI1_4(%rip), %xmm8
	vmovsd	.LCPI1_3(%rip), %xmm7
	vmovsd	.LCPI1_5(%rip), %xmm15
	vmovsd	.LCPI1_1(%rip), %xmm14
	vmovups	304(%rsp), %ymm13
	vmovups	208(%rsp), %ymm12
	vmovaps	176(%rsp), %xmm11
	movq	488(%rsp), %r8
	movq	496(%rsp), %r9
	movq	504(%rsp), %r11
	movq	472(%rsp), %rbp
	movl	148(%rsp), %esi
	vmovaps	192(%rsp), %xmm10
	addq	$112, %rsp
	.cfi_adjust_cfa_offset -112
	testl	%eax, %eax
	je	.LBB1_4
	jmp	.LBB1_30
	.p2align	4, 0x90
.LBB1_16:
	movq	184(%rsp), %rdx
	movq	%rax, 192(%rsp)
	movq	%rdi, %rax
	movq	%rbp, 96(%rsp)
	notq	%rax
	addq	%rcx, %rax
	leal	(%rdx,%rbx), %edx
	leal	(%rdx,%rax), %ebp
	cmpl	%edx, %ebp
	jl	.LBB1_26
	movq	%rax, %rdx
	shrq	$32, %rdx
	jne	.LBB1_26
	movq	80(%rsp), %rbp
	leal	(%rbx,%rbp), %r8d
	leal	(%r8,%rax), %ebp
	cmpl	%r8d, %ebp
	jl	.LBB1_27
	movq	96(%rsp), %rbp
	testq	%rdx, %rdx
	jne	.LBB1_28
	leal	(%rbx,%rbp), %r9d
	leal	(%r9,%rax), %eax
	cmpl	%r9d, %eax
	jl	.LBB1_25
	testq	%rdx, %rdx
	jne	.LBB1_25
	movq	192(%rsp), %rsi
	vmovdqu	224(%rsp), %ymm9
	movl	%r8d, %ebp
	movl	%r9d, %eax
	leaq	(%r10,%rdi,4), %r8
	addl	%r12d, %ebx
	leaq	(%r11,%rdi,4), %r9
	xorl	%r10d, %r10d
	andq	$-8, %rsi
	leaq	(%rsi,%rdi), %rdx
	.p2align	4, 0x90
.LBB1_23:
	vpaddd	(%r8,%r10,4), %ymm9, %ymm2
	leal	(%rbp,%r10), %edi
	leal	(%rbx,%r10), %r11d
	movslq	%edi, %rdi
	movslq	%r11d, %r11
	vmovups	(%r14,%rdi,4), %ymm8
	leal	(%rax,%r10), %edi
	movslq	%edi, %rdi
	vmovups	(%r14,%rdi,4), %ymm1
	vextracti128	$1, %ymm2, %xmm3
	vpmovsxdq	%xmm2, %ymm2
	vmovq	%xmm2, %rdi
	vextracti128	$1, %ymm2, %xmm6
	vpmovsxdq	%xmm3, %ymm3
	vmovss	(%r14,%rdi,4), %xmm4
	vmovq	%xmm3, %rdi
	vmovss	(%r14,%rdi,4), %xmm5
	vpextrq	$1, %xmm3, %rdi
	vinsertps	$16, (%r14,%rdi,4), %xmm5, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm3, %xmm2
	vinsertps	$16, (%r14,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%r14,%rdi,4), %xmm5, %xmm3
	vpaddd	(%r9,%r10,4), %ymm9, %ymm5
	vmovq	%xmm6, %rdi
	addq	$8, %r10
	cmpq	%r10, %rsi
	vinsertps	$32, (%r14,%rdi,4), %xmm4, %xmm4
	vpmovsxdq	%xmm5, %ymm7
	vextracti128	$1, %ymm5, %xmm5
	vmovq	%xmm7, %rdi
	vpmovsxdq	%xmm5, %ymm5
	vmovss	(%r14,%rdi,4), %xmm0
	vpextrq	$1, %xmm6, %rdi
	vinsertps	$48, (%r14,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm5, %rdi
	vmovss	(%r14,%rdi,4), %xmm6
	vpextrq	$1, %xmm5, %rdi
	vextracti128	$1, %ymm5, %xmm5
	vinsertps	$16, (%r14,%rdi,4), %xmm6, %xmm6
	vmovq	%xmm5, %rdi
	vinsertps	$32, (%r14,%rdi,4), %xmm6, %xmm6
	vpextrq	$1, %xmm7, %rdi
	vinsertps	$16, (%r14,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm7, %xmm2
	vinsertps	$48, (%r14,%rdi,4), %xmm3, %xmm3
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%r14,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm5, %rdi
	vinsertps	$48, (%r14,%rdi,4), %xmm6, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vmovups	(%r14,%r11,4), %ymm2
	vinsertps	$48, (%r14,%rdi,4), %xmm0, %xmm0
	movq	336(%rsp), %rdi
	vinsertf128	$1, %xmm3, %ymm4, %ymm3
	vsubps	%ymm2, %ymm8, %ymm6
	vsubps	%ymm2, %ymm1, %ymm1
	vsubps	%ymm2, %ymm3, %ymm3
	vinsertf128	$1, %xmm5, %ymm0, %ymm0
	vmovups	%ymm6, (%rdi,%r11,4)
	movq	344(%rsp), %rdi
	vmulps	%ymm6, %ymm6, %ymm4
	vmulps	%ymm1, %ymm1, %ymm5
	vsubps	%ymm2, %ymm0, %ymm0
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm3, %ymm3, %ymm5
	vmovups	%ymm1, (%rdi,%r11,4)
	movq	352(%rsp), %rdi
	vaddps	%ymm1, %ymm6, %ymm1
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm0, %ymm0, %ymm5
	vmulps	%ymm2, %ymm2, %ymm6
	vaddps	%ymm3, %ymm1, %ymm1
	vaddps	%ymm5, %ymm4, %ymm4
	vmovups	%ymm3, (%rdi,%r11,4)
	movq	360(%rsp), %rdi
	vbroadcastsd	.LCPI1_1(%rip), %ymm3
	vmovups	%ymm0, (%rdi,%r11,4)
	vaddps	%ymm0, %ymm1, %ymm0
	vdivps	%ymm6, %ymm4, %ymm1
	vbroadcastsd	.LCPI1_2(%rip), %ymm6
	vdivps	%ymm2, %ymm0, %ymm0
	vextractf128	$1, %ymm1, %xmm2
	vcvtps2pd	%xmm1, %ymm1
	vcvtps2pd	%xmm2, %ymm2
	vmulps	%ymm0, %ymm0, %ymm4
	vmulpd	%ymm3, %ymm1, %ymm1
	vmulpd	%ymm3, %ymm2, %ymm2
	vextractf128	$1, %ymm4, %xmm5
	vcvtps2pd	%xmm4, %ymm4
	vcvtps2pd	%xmm5, %ymm5
	vmulpd	%ymm6, %ymm4, %ymm4
	vmulpd	%ymm6, %ymm5, %ymm3
	vsubpd	%ymm4, %ymm1, %ymm1
	vextractf128	$1, %ymm0, %xmm4
	vcvtps2pd	%xmm0, %ymm0
	vsubpd	%ymm3, %ymm2, %ymm2
	vcvtps2pd	%xmm4, %ymm3
	vbroadcastsd	.LCPI1_3(%rip), %ymm4
	vcvtpd2ps	%ymm1, %xmm1
	vcvtpd2ps	%ymm2, %xmm2
	vmulpd	%ymm4, %ymm0, %ymm0
	vmulpd	%ymm4, %ymm3, %ymm3
	vbroadcastsd	.LCPI1_4(%rip), %ymm4
	vinsertf128	$1, %xmm2, %ymm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm3, %ymm3
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm3, %xmm2
	vinsertf128	$1, %xmm2, %ymm0, %ymm0
	vbroadcastss	.LCPI1_0(%rip), %ymm2
	vmulps	%ymm0, %ymm0, %ymm0
	vdivps	%ymm0, %ymm1, %ymm0
	vsubps	%ymm12, %ymm0, %ymm0
	vdivps	%ymm13, %ymm0, %ymm0
	vextractf128	$1, %ymm0, %xmm1
	vcvtps2pd	%xmm0, %ymm0
	vcvtps2pd	%xmm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm1, %ymm1
	vdivpd	%ymm0, %ymm4, %ymm0
	vdivpd	%ymm1, %ymm4, %ymm1
	vxorpd	%xmm4, %xmm4, %xmm4
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm1, %xmm1
	vinsertf128	$1, %xmm1, %ymm0, %ymm0
	vmovupd	%ymm0, (%r15,%r11,4)
	vcmpltps	%ymm4, %ymm0, %ymm1
	vcmpnltps	%ymm4, %ymm0, %ymm3
	vcmpltps	%ymm0, %ymm2, %ymm0
	vandps	%ymm3, %ymm0, %ymm0
	vmaskmovps	%ymm2, %ymm0, (%r15,%r11,4)
	vmaskmovps	%ymm4, %ymm1, (%r15,%r11,4)
	jne	.LBB1_23
	cmpq	%rsi, 192(%rsp)
	movl	36(%rsp), %esi
	movq	400(%rsp), %r10
	movq	392(%rsp), %r11
	movq	384(%rsp), %r9
	movq	376(%rsp), %r8
	vmovsd	.LCPI1_3(%rip), %xmm7
	vmovsd	.LCPI1_4(%rip), %xmm8
	movq	96(%rsp), %rbp
	jne	.LBB1_7
	jmp	.LBB1_12
.LBB1_25:
	movq	384(%rsp), %r9
	movq	376(%rsp), %r8
	jmp	.LBB1_6
.LBB1_26:
	movq	96(%rsp), %rbp
	jmp	.LBB1_6
.LBB1_27:
	movq	376(%rsp), %r8
	movq	96(%rsp), %rbp
	jmp	.LBB1_6
.LBB1_28:
	movq	376(%rsp), %r8
	jmp	.LBB1_6
	.p2align	4, 0x90
.LBB1_29:
	movq	40(%rsp), %rcx
	movq	24(%rsp), %rax
	addq	168(%rsp), %r12
	incq	%rax
	incl	%ecx
	cmpq	176(%rsp), %rax
	movq	%rcx, 40(%rsp)
	movq	%rax, 24(%rsp)
	jl	.LBB1_2
.LBB1_30:
	addq	$264, %rsp
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
	.size	_Z11srad_tpal_1iiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f, .Lfunc_end1-_Z11srad_tpal_1iiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.cfi_endproc

	.section	.rodata.cst4,"aM",@progbits,4
	.p2align	2
.LCPI2_0:
	.long	1065353216
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3
.LCPI2_1:
	.quad	4602678819172646912
.LCPI2_2:
	.quad	4589168020290535424
.LCPI2_3:
	.quad	4598175219545276416
.LCPI2_4:
	.quad	4607182418800017408
.LCPI2_5:
	.quad	-4634204016564240384
	.text
	.globl	_Z17srad_tpal_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.p2align	4, 0x90
	.type	_Z17srad_tpal_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f,@function
_Z17srad_tpal_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f:
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
	subq	$200, %rsp
	.cfi_def_cfa_offset 256
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpl	%r9d, %r8d
	movl	%ecx, 56(%rsp)
	movl	%edx, 8(%rsp)
	movl	%edi, 52(%rsp)
	jge	.LBB2_30
	vmovaps	%xmm0, %xmm10
	vaddss	.LCPI2_0(%rip), %xmm0, %xmm0
	movl	8(%rsp), %eax
	movl	%r9d, %r14d
	movl	%r8d, %ebp
	movq	352(%rsp), %r11
	movq	344(%rsp), %r9
	movq	336(%rsp), %r10
	movq	328(%rsp), %r8
	movq	320(%rsp), %r15
	movq	304(%rsp), %r12
	movq	280(%rsp), %r13
	vmovsd	.LCPI2_1(%rip), %xmm15
	vmovsd	.LCPI2_5(%rip), %xmm7
	vmovsd	.LCPI2_3(%rip), %xmm8
	vmovaps	%xmm1, %xmm9
	movl	%esi, 12(%rsp)
	vbroadcastss	%xmm10, %ymm13
	movl	%r14d, 60(%rsp)
	movl	%eax, %ebx
	cltq
	imull	%esi, %ebx
	movq	%rax, 88(%rsp)
	vmulss	%xmm10, %xmm0, %xmm11
	vmovd	%ebx, %xmm0
	vbroadcastss	%xmm11, %ymm14
	vpbroadcastd	%xmm0, %ymm12
	.p2align	4, 0x90
.LBB2_2:
	leal	64(%rbp), %ecx
	cmpl	%r14d, %ecx
	cmovgl	%r14d, %ecx
	cmpl	%ecx, %ebp
	jge	.LBB2_24
	movq	88(%rsp), %rax
	movslq	%ecx, %r12
	movslq	%ebp, %rdi
	movl	%ecx, 64(%rsp)
	movl	(%r8,%rax,4), %edx
	movl	(%r10,%rax,4), %eax
	imull	%esi, %edx
	imull	%esi, %eax
	movq	%rdx, 40(%rsp)
	movq	%r12, %rdx
	subq	%rdi, %rdx
	cmpq	$8, %rdx
	jae	.LBB2_5
	movq	%rdi, %rdx
	jmp	.LBB2_18
	.p2align	4, 0x90
.LBB2_5:
	movq	%rdx, 96(%rsp)
	movq	%rdi, %rdx
	leal	(%rbx,%rbp), %r8d
	movl	%eax, 16(%rsp)
	notq	%rdx
	addq	%r12, %rdx
	leal	(%r8,%rdx), %eax
	cmpl	%r8d, %eax
	jl	.LBB2_16
	movq	%rdx, %r10
	shrq	$32, %r10
	jne	.LBB2_15
	movq	40(%rsp), %rax
	leal	(%rbp,%rax), %r9d
	leal	(%r9,%rdx), %eax
	cmpl	%r9d, %eax
	jl	.LBB2_14
	movl	16(%rsp), %eax
	testq	%r10, %r10
	jne	.LBB2_29
	addl	%eax, %ebp
	leal	(%rbp,%rdx), %eax
	cmpl	%ebp, %eax
	jl	.LBB2_14
	testq	%r10, %r10
	jne	.LBB2_14
	movq	344(%rsp), %rcx
	movq	96(%rsp), %rsi
	movl	%r9d, %eax
	movl	%r8d, %r14d
	movl	%ebp, %ebp
	leaq	(%r11,%rdi,4), %r8
	xorl	%r10d, %r10d
	leaq	(%rcx,%rdi,4), %r9
	movq	288(%rsp), %rcx
	andq	$-8, %rsi
	leaq	(%rsi,%rdi), %rdx
	.p2align	4, 0x90
.LBB2_12:
	vpaddd	(%r8,%r10,4), %ymm12, %ymm2
	leal	(%rax,%r10), %edi
	leal	(%r14,%r10), %r11d
	movslq	%edi, %rdi
	movslq	%r11d, %r11
	vmovups	(%r13,%rdi,4), %ymm8
	leal	(%rbp,%r10), %edi
	movslq	%edi, %rdi
	vmovups	(%r13,%rdi,4), %ymm1
	vextracti128	$1, %ymm2, %xmm3
	vpmovsxdq	%xmm2, %ymm2
	vmovq	%xmm2, %rdi
	vextracti128	$1, %ymm2, %xmm6
	vpmovsxdq	%xmm3, %ymm3
	vmovss	(%r13,%rdi,4), %xmm4
	vmovq	%xmm3, %rdi
	vmovss	(%r13,%rdi,4), %xmm5
	vpextrq	$1, %xmm3, %rdi
	vinsertps	$16, (%r13,%rdi,4), %xmm5, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm3, %xmm2
	vinsertps	$16, (%r13,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%r13,%rdi,4), %xmm5, %xmm3
	vpaddd	(%r9,%r10,4), %ymm12, %ymm5
	vmovq	%xmm6, %rdi
	addq	$8, %r10
	cmpq	%r10, %rsi
	vinsertps	$32, (%r13,%rdi,4), %xmm4, %xmm4
	vpmovsxdq	%xmm5, %ymm7
	vextracti128	$1, %ymm5, %xmm5
	vmovq	%xmm7, %rdi
	vpmovsxdq	%xmm5, %ymm5
	vmovss	(%r13,%rdi,4), %xmm0
	vpextrq	$1, %xmm6, %rdi
	vinsertps	$48, (%r13,%rdi,4), %xmm4, %xmm4
	vmovq	%xmm5, %rdi
	vmovss	(%r13,%rdi,4), %xmm6
	vpextrq	$1, %xmm5, %rdi
	vextracti128	$1, %ymm5, %xmm5
	vinsertps	$16, (%r13,%rdi,4), %xmm6, %xmm6
	vmovq	%xmm5, %rdi
	vinsertps	$32, (%r13,%rdi,4), %xmm6, %xmm6
	vpextrq	$1, %xmm7, %rdi
	vinsertps	$16, (%r13,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm2, %rdi
	vextracti128	$1, %ymm7, %xmm2
	vinsertps	$48, (%r13,%rdi,4), %xmm3, %xmm3
	vmovq	%xmm2, %rdi
	vinsertps	$32, (%r13,%rdi,4), %xmm0, %xmm0
	vpextrq	$1, %xmm5, %rdi
	vinsertps	$48, (%r13,%rdi,4), %xmm6, %xmm5
	vpextrq	$1, %xmm2, %rdi
	vmovups	(%r13,%r11,4), %ymm2
	vinsertps	$48, (%r13,%rdi,4), %xmm0, %xmm0
	movq	296(%rsp), %rdi
	vinsertf128	$1, %xmm3, %ymm4, %ymm3
	vsubps	%ymm2, %ymm1, %ymm1
	vsubps	%ymm2, %ymm8, %ymm6
	vsubps	%ymm2, %ymm3, %ymm3
	vinsertf128	$1, %xmm5, %ymm0, %ymm0
	vmovups	%ymm1, (%rdi,%r11,4)
	movq	304(%rsp), %rdi
	vmulps	%ymm1, %ymm1, %ymm5
	vmulps	%ymm6, %ymm6, %ymm4
	vaddps	%ymm1, %ymm6, %ymm1
	vmovups	%ymm6, (%rcx,%r11,4)
	vmulps	%ymm2, %ymm2, %ymm6
	vsubps	%ymm2, %ymm0, %ymm0
	vaddps	%ymm3, %ymm1, %ymm1
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm3, %ymm3, %ymm5
	vmovups	%ymm3, (%rdi,%r11,4)
	movq	312(%rsp), %rdi
	vaddps	%ymm5, %ymm4, %ymm4
	vmulps	%ymm0, %ymm0, %ymm5
	vbroadcastsd	.LCPI2_1(%rip), %ymm3
	vaddps	%ymm5, %ymm4, %ymm4
	vmovups	%ymm0, (%rdi,%r11,4)
	vaddps	%ymm0, %ymm1, %ymm0
	vdivps	%ymm6, %ymm4, %ymm1
	vbroadcastsd	.LCPI2_2(%rip), %ymm6
	vdivps	%ymm2, %ymm0, %ymm0
	vextractf128	$1, %ymm1, %xmm2
	vcvtps2pd	%xmm1, %ymm1
	vcvtps2pd	%xmm2, %ymm2
	vmulps	%ymm0, %ymm0, %ymm4
	vmulpd	%ymm3, %ymm1, %ymm1
	vmulpd	%ymm3, %ymm2, %ymm2
	vextractf128	$1, %ymm4, %xmm5
	vcvtps2pd	%xmm4, %ymm4
	vcvtps2pd	%xmm5, %ymm5
	vmulpd	%ymm6, %ymm4, %ymm4
	vmulpd	%ymm6, %ymm5, %ymm3
	vsubpd	%ymm4, %ymm1, %ymm1
	vextractf128	$1, %ymm0, %xmm4
	vcvtps2pd	%xmm0, %ymm0
	vsubpd	%ymm3, %ymm2, %ymm2
	vcvtps2pd	%xmm4, %ymm3
	vbroadcastsd	.LCPI2_3(%rip), %ymm4
	vcvtpd2ps	%ymm1, %xmm1
	vcvtpd2ps	%ymm2, %xmm2
	vmulpd	%ymm4, %ymm0, %ymm0
	vmulpd	%ymm4, %ymm3, %ymm3
	vbroadcastsd	.LCPI2_4(%rip), %ymm4
	vinsertf128	$1, %xmm2, %ymm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm3, %ymm3
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm3, %xmm2
	vinsertf128	$1, %xmm2, %ymm0, %ymm0
	vbroadcastss	.LCPI2_0(%rip), %ymm2
	vmulps	%ymm0, %ymm0, %ymm0
	vdivps	%ymm0, %ymm1, %ymm0
	vsubps	%ymm13, %ymm0, %ymm0
	vdivps	%ymm14, %ymm0, %ymm0
	vextractf128	$1, %ymm0, %xmm1
	vcvtps2pd	%xmm0, %ymm0
	vcvtps2pd	%xmm1, %ymm1
	vaddpd	%ymm4, %ymm0, %ymm0
	vaddpd	%ymm4, %ymm1, %ymm1
	vdivpd	%ymm0, %ymm4, %ymm0
	vdivpd	%ymm1, %ymm4, %ymm1
	vxorpd	%xmm4, %xmm4, %xmm4
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm1, %xmm1
	vinsertf128	$1, %xmm1, %ymm0, %ymm0
	vmovupd	%ymm0, (%r15,%r11,4)
	vcmpltps	%ymm4, %ymm0, %ymm1
	vcmpnltps	%ymm4, %ymm0, %ymm3
	vcmpltps	%ymm0, %ymm2, %ymm0
	vandps	%ymm3, %ymm0, %ymm0
	vmaskmovps	%ymm2, %ymm0, (%r15,%r11,4)
	vmaskmovps	%ymm4, %ymm1, (%r15,%r11,4)
	jne	.LBB2_12
	cmpq	%rsi, 96(%rsp)
	movl	60(%rsp), %r14d
	movl	12(%rsp), %esi
	movq	352(%rsp), %r11
	movq	344(%rsp), %r9
	movq	336(%rsp), %r10
	movq	328(%rsp), %r8
	vmovsd	.LCPI2_5(%rip), %xmm7
	vmovsd	.LCPI2_3(%rip), %xmm8
	movl	16(%rsp), %eax
	jne	.LBB2_18
	jmp	.LBB2_23
.LBB2_14:
	movq	344(%rsp), %r9
.LBB2_15:
	movq	336(%rsp), %r10
.LBB2_16:
	movq	%rdi, %rdx
	movq	328(%rsp), %r8
	movl	16(%rsp), %eax
.LBB2_18:
	movl	40(%rsp), %ecx
	movl	%eax, %eax
	.p2align	4, 0x90
.LBB2_19:
	leal	(%rbx,%rdx), %edi
	leal	(%rcx,%rdx), %ebp
	movslq	%edi, %rdi
	movslq	%ebp, %rbp
	vmovss	(%r13,%rbp,4), %xmm1
	vmovss	(%r13,%rdi,4), %xmm0
	movq	288(%rsp), %rbp
	vsubss	%xmm0, %xmm1, %xmm1
	vmovss	%xmm1, (%rbp,%rdi,4)
	leal	(%rax,%rdx), %ebp
	vmulss	%xmm1, %xmm1, %xmm5
	movslq	%ebp, %rbp
	vmovss	(%r13,%rbp,4), %xmm2
	movq	296(%rsp), %rbp
	vsubss	%xmm0, %xmm2, %xmm2
	vmovss	%xmm2, (%rbp,%rdi,4)
	movl	(%r11,%rdx,4), %ebp
	vmulss	%xmm2, %xmm2, %xmm6
	vaddss	%xmm2, %xmm1, %xmm1
	vaddss	%xmm6, %xmm5, %xmm5
	addl	%ebx, %ebp
	movslq	%ebp, %rbp
	vmovss	(%r13,%rbp,4), %xmm3
	movq	304(%rsp), %rbp
	vsubss	%xmm0, %xmm3, %xmm3
	vmovss	%xmm3, (%rbp,%rdi,4)
	movl	(%r9,%rdx,4), %ebp
	vmulss	%xmm3, %xmm3, %xmm6
	vaddss	%xmm3, %xmm1, %xmm1
	vaddss	%xmm6, %xmm5, %xmm5
	addl	%ebx, %ebp
	movslq	%ebp, %rbp
	vmovss	(%r13,%rbp,4), %xmm4
	movq	312(%rsp), %rbp
	vsubss	%xmm0, %xmm4, %xmm4
	vmulss	%xmm4, %xmm4, %xmm6
	vaddss	%xmm4, %xmm1, %xmm1
	vmovss	%xmm4, (%rbp,%rdi,4)
	vaddss	%xmm6, %xmm5, %xmm5
	vmulss	%xmm0, %xmm0, %xmm6
	vdivss	%xmm0, %xmm1, %xmm0
	vdivss	%xmm6, %xmm5, %xmm5
	vmulss	%xmm0, %xmm0, %xmm2
	vcvtss2sd	%xmm5, %xmm5, %xmm1
	vcvtss2sd	%xmm2, %xmm2, %xmm2
	vmulsd	%xmm15, %xmm1, %xmm1
	vmulsd	%xmm7, %xmm2, %xmm2
	vaddsd	%xmm2, %xmm1, %xmm1
	vmovsd	.LCPI2_4(%rip), %xmm2
	vcvtsd2ss	%xmm1, %xmm1, %xmm1
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	%xmm8, %xmm0, %xmm0
	vaddsd	%xmm2, %xmm0, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm0
	vmulss	%xmm0, %xmm0, %xmm0
	vdivss	%xmm0, %xmm1, %xmm0
	vsubss	%xmm10, %xmm0, %xmm0
	vdivss	%xmm11, %xmm0, %xmm0
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vaddsd	%xmm2, %xmm0, %xmm0
	vdivsd	%xmm0, %xmm2, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm1
	vxorpd	%xmm0, %xmm0, %xmm0
	vucomiss	%xmm1, %xmm0
	vmovss	%xmm1, (%r15,%rdi,4)
	ja	.LBB2_21
	vmovss	.LCPI2_0(%rip), %xmm0
	vucomiss	%xmm0, %xmm1
	jbe	.LBB2_22
.LBB2_21:
	vmovss	%xmm0, (%r15,%rdi,4)
.LBB2_22:
	incq	%rdx
	cmpq	%r12, %rdx
	jl	.LBB2_19
.LBB2_23:
	movl	64(%rsp), %eax
	movq	304(%rsp), %r12
	movl	%eax, %ebp
.LBB2_24:
	cmpl	%r14d, %ebp
	jge	.LBB2_30
	movq	heartbeat@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	je	.LBB2_2
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movl	60(%rsp), %edi
	movl	16(%rsp), %edx
	movl	64(%rsp), %ecx
	movl	%ebp, %r8d
	movl	%r14d, %r9d
	vmovaps	%xmm10, %xmm0
	vmovaps	%xmm9, %xmm1
	pushq	%r11
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r10
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r15
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r12
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r13
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	movl	360(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	movl	360(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	vmovss	%xmm9, 152(%rsp)
	vmovaps	%xmm10, 176(%rsp)
	movq	%r11, %r12
	vmovaps	%xmm11, 128(%rsp)
	vmovdqu	%ymm12, 208(%rsp)
	vmovups	%ymm13, 272(%rsp)
	vmovups	%ymm14, 240(%rsp)
	vzeroupper
	callq	_Z20srad_handler_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f@PLT
	movq	%r12, %r11
	vmovsd	.LCPI2_3(%rip), %xmm8
	vmovsd	.LCPI2_5(%rip), %xmm7
	vmovsd	.LCPI2_1(%rip), %xmm15
	vmovups	240(%rsp), %ymm14
	vmovups	272(%rsp), %ymm13
	vmovdqu	208(%rsp), %ymm12
	vmovaps	128(%rsp), %xmm11
	movq	440(%rsp), %r8
	movq	448(%rsp), %r10
	movq	456(%rsp), %r9
	movq	416(%rsp), %r12
	movl	124(%rsp), %esi
	vmovaps	176(%rsp), %xmm10
	vmovss	152(%rsp), %xmm9
	addq	$112, %rsp
	.cfi_adjust_cfa_offset -112
	testl	%eax, %eax
	je	.LBB2_2
	jmp	.LBB2_30
.LBB2_29:
	movq	344(%rsp), %r9
	movq	336(%rsp), %r10
	movq	328(%rsp), %r8
	movq	%rdi, %rdx
	jmp	.LBB2_18
.LBB2_30:
	addq	$200, %rsp
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
.Lfunc_end2:
	.size	_Z17srad_tpal_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f, .Lfunc_end2-_Z17srad_tpal_inner_1iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.cfi_endproc

	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3
.LCPI3_0:
	.quad	4598175219545276416
	.text
	.globl	_Z11srad_tpal_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.p2align	4, 0x90
	.type	_Z11srad_tpal_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f,@function
_Z11srad_tpal_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f:
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
	subq	$200, %rsp
	.cfi_def_cfa_offset 256
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpl	%ecx, %edx
	movl	%edi, 64(%rsp)
	movl	%ecx, 36(%rsp)
	movq	%rdx, %rax
	movq	%rdx, 72(%rsp)
	jge	.LBB3_24
	testl	%esi, %esi
	jle	.LBB3_24
	vmovapd	%xmm0, %xmm6
	vcvtss2sd	%xmm1, %xmm1, %xmm0
	vmulsd	.LCPI3_0(%rip), %xmm0, %xmm7
	movq	72(%rsp), %rax
	movq	344(%rsp), %r8
	movq	320(%rsp), %r12
	movq	312(%rsp), %rbp
	movq	304(%rsp), %r9
	movq	296(%rsp), %r15
	movq	288(%rsp), %rcx
	movq	280(%rsp), %r14
	vmovapd	%xmm1, %xmm5
	movl	%esi, 12(%rsp)
	movl	%eax, %r13d
	movslq	%eax, %rdx
	movslq	36(%rsp), %rax
	imull	%esi, %r13d
	movq	%rdx, 16(%rsp)
	vbroadcastsd	%xmm7, %ymm8
	movq	%rax, 112(%rsp)
	movl	%esi, %eax
	movq	%rax, 104(%rsp)
	xorl	%eax, %eax
	movq	%rax, 40(%rsp)
	.p2align	4, 0x90
.LBB3_3:
	movq	72(%rsp), %rax
	movq	40(%rsp), %rdi
	movq	%r13, 56(%rsp)
	leal	(%rdi,%rax), %eax
	imull	%esi, %eax
	movq	%rax, 120(%rsp)
	movq	16(%rsp), %rax
	imull	%esi, %eax
	vmovd	%eax, %xmm0
	movl	%eax, 68(%rsp)
	xorl	%eax, %eax
	movq	%rax, 24(%rsp)
	vpbroadcastd	%xmm0, %ymm9
	.p2align	4, 0x90
.LBB3_4:
	movq	24(%rsp), %rdi
	leal	64(%rdi), %ebx
	cmpl	%esi, %ebx
	cmovgl	%esi, %ebx
	cmpl	%ebx, %edi
	jge	.LBB3_20
	movq	336(%rsp), %rax
	movq	16(%rsp), %rdx
	movslq	%ebx, %r13
	movslq	%edi, %r10
	movl	%ebx, 8(%rsp)
	movl	(%rax,%rdx,4), %r11d
	movq	%r13, %rax
	subq	%r10, %rax
	imull	%esi, %r11d
	cmpq	$7, %rax
	jbe	.LBB3_16
	movq	120(%rsp), %rcx
	movq	%r10, %rdx
	notq	%rdx
	addq	%r13, %rdx
	leal	(%rcx,%rdi), %ecx
	leal	(%rcx,%rdx), %edi
	cmpl	%ecx, %edi
	jl	.LBB3_15
	movq	%rdx, %rdi
	shrq	$32, %rdi
	jne	.LBB3_15
	movq	24(%rsp), %rcx
	leal	(%rcx,%r11), %ecx
	leal	(%rcx,%rdx), %edx
	cmpl	%ecx, %edx
	jl	.LBB3_15
	testq	%rdi, %rdi
	jne	.LBB3_15
	movq	24(%rsp), %rdx
	movq	%r11, 48(%rsp)
	movl	%ecx, %r11d
	movq	288(%rsp), %rcx
	movq	%rax, %rdi
	movq	%rax, 80(%rsp)
	movq	%r10, %rax
	movq	%r9, %r15
	xorl	%ebx, %ebx
	andq	$-8, %rdi
	leaq	(%r8,%rax,4), %r9
	leaq	(%rdi,%r10), %r10
	addl	56(%rsp), %edx
	movq	%rdx, %rsi
	.p2align	4, 0x90
.LBB3_12:
	vpaddd	(%r9,%rbx,4), %ymm9, %ymm1
	leal	(%r11,%rbx), %r8d
	leal	(%rsi,%rbx), %edx
	addq	$8, %rbx
	movslq	%r8d, %rax
	movq	296(%rsp), %r8
	movslq	%edx, %rdx
	cmpq	%rbx, %rdi
	vmovups	(%r12,%rax,4), %ymm0
	vextracti128	$1, %ymm1, %xmm2
	vpmovsxdq	%xmm1, %ymm1
	vmovq	%xmm1, %rax
	vmulps	(%r8,%rdx,4), %ymm0, %ymm0
	vpmovsxdq	%xmm2, %ymm2
	vmovss	(%r12,%rax,4), %xmm3
	vmovq	%xmm2, %rax
	vmovss	(%r12,%rax,4), %xmm4
	vpextrq	$1, %xmm2, %rax
	vextracti128	$1, %ymm2, %xmm2
	vinsertps	$16, (%r12,%rax,4), %xmm4, %xmm4
	vmovq	%xmm2, %rax
	vinsertps	$32, (%r12,%rax,4), %xmm4, %xmm4
	vpextrq	$1, %xmm2, %rax
	vmovups	(%r12,%rdx,4), %ymm2
	vinsertps	$48, (%r12,%rax,4), %xmm4, %xmm4
	vpextrq	$1, %xmm1, %rax
	vextracti128	$1, %ymm1, %xmm1
	vinsertps	$16, (%r12,%rax,4), %xmm3, %xmm3
	vmovq	%xmm1, %rax
	vinsertps	$32, (%r12,%rax,4), %xmm3, %xmm3
	vpextrq	$1, %xmm1, %rax
	vmulps	(%rcx,%rdx,4), %ymm2, %ymm1
	vinsertps	$48, (%r12,%rax,4), %xmm3, %xmm3
	vaddps	%ymm0, %ymm1, %ymm0
	vmulps	(%r15,%rdx,4), %ymm2, %ymm1
	vcvtps2pd	(%r14,%rdx,4), %ymm2
	vaddps	%ymm1, %ymm0, %ymm0
	vinsertf128	$1, %xmm4, %ymm3, %ymm1
	vmulps	(%rbp,%rdx,4), %ymm1, %ymm1
	vaddps	%ymm1, %ymm0, %ymm0
	vcvtps2pd	16(%r14,%rdx,4), %ymm1
	vextractf128	$1, %ymm0, %xmm3
	vcvtps2pd	%xmm0, %ymm0
	vcvtps2pd	%xmm3, %ymm3
	vmulpd	%ymm0, %ymm8, %ymm0
	vmulpd	%ymm3, %ymm8, %ymm3
	vaddpd	%ymm2, %ymm0, %ymm0
	vaddpd	%ymm1, %ymm3, %ymm1
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm1, %xmm1
	vinsertf128	$1, %xmm1, %ymm0, %ymm0
	vmovupd	%ymm0, (%r14,%rdx,4)
	jne	.LBB3_12
	movq	%r15, %r9
	movl	12(%rsp), %edx
	movq	344(%rsp), %r8
	movq	296(%rsp), %r15
	movq	48(%rsp), %r11
	cmpq	%rdi, 80(%rsp)
	jne	.LBB3_17
	movl	8(%rsp), %eax
	movq	56(%rsp), %r13
	movl	%edx, %esi
	movl	%eax, %edi
	cmpl	%esi, %edi
	movq	%rdi, 24(%rsp)
	jge	.LBB3_23
	.p2align	4, 0x90
.LBB3_21:
	movq	heartbeat@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	je	.LBB3_4
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movq	24(%rsp), %rdx
	movq	32(%rsp), %r8
	movq	%r9, %rbx
	movl	44(%rsp), %ecx
	movl	20(%rsp), %r9d
	movl	72(%rsp), %edi
	vmovapd	%xmm6, %xmm0
	vmovapd	%xmm5, %xmm1
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r12
	.cfi_adjust_cfa_offset 8
	pushq	%rbp
	.cfi_adjust_cfa_offset 8
	pushq	%rbx
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r14
	.cfi_adjust_cfa_offset 8
	pushq	360(%rsp)
	.cfi_adjust_cfa_offset 8
	movl	360(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	movl	360(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	vmovss	%xmm5, 120(%rsp)
	vmovss	%xmm6, 160(%rsp)
	vmovapd	%xmm7, 192(%rsp)
	vmovupd	%ymm8, 272(%rsp)
	vmovdqu	%ymm9, 240(%rsp)
	vzeroupper
	callq	_Z14srad_handler_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f@PLT
	vmovdqu	240(%rsp), %ymm9
	vmovupd	272(%rsp), %ymm8
	vmovapd	192(%rsp), %xmm7
	movq	400(%rsp), %rcx
	movq	408(%rsp), %r15
	movq	456(%rsp), %r8
	movl	124(%rsp), %esi
	vmovss	160(%rsp), %xmm6
	vmovss	120(%rsp), %xmm5
	movq	%rbx, %r9
	addq	$112, %rsp
	.cfi_adjust_cfa_offset -112
	testl	%eax, %eax
	je	.LBB3_4
	jmp	.LBB3_24
.LBB3_15:
	movq	288(%rsp), %rcx
.LBB3_16:
	movl	%esi, %edx
.LBB3_17:
	movl	%r11d, %eax
	movq	56(%rsp), %r11
	movl	68(%rsp), %esi
	.p2align	4, 0x90
.LBB3_18:
	leal	(%r11,%r10), %edi
	movslq	%edi, %rbx
	leal	(%rax,%r10), %edi
	movslq	%edi, %rdi
	vmovss	(%r12,%rbx,4), %xmm0
	vmovss	(%r12,%rdi,4), %xmm1
	movl	(%r8,%r10,4), %edi
	incq	%r10
	vmulss	(%rcx,%rbx,4), %xmm0, %xmm3
	vmulss	(%r9,%rbx,4), %xmm0, %xmm0
	vmulss	(%r15,%rbx,4), %xmm1, %xmm1
	addl	%esi, %edi
	cmpq	%r13, %r10
	movslq	%edi, %rdi
	vmovss	(%r12,%rdi,4), %xmm2
	vaddss	%xmm1, %xmm3, %xmm1
	vaddss	%xmm0, %xmm1, %xmm0
	vmulss	(%rbp,%rbx,4), %xmm2, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	(%r14,%rbx,4), %xmm1
	vcvtss2sd	%xmm1, %xmm1, %xmm1
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	%xmm0, %xmm7, %xmm0
	vaddsd	%xmm1, %xmm0, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm0
	vmovd	%xmm0, (%r14,%rbx,4)
	jl	.LBB3_18
	movl	8(%rsp), %eax
	movq	%r11, %r13
	movl	%edx, %esi
	movl	%eax, %edi
.LBB3_20:
	cmpl	%esi, %edi
	movq	%rdi, 24(%rsp)
	jl	.LBB3_21
.LBB3_23:
	movq	16(%rsp), %rax
	addq	104(%rsp), %r13
	movq	%rax, %rdx
	movq	40(%rsp), %rax
	incq	%rdx
	movq	%rdx, 16(%rsp)
	incl	%eax
	cmpq	112(%rsp), %rdx
	movq	%rax, 40(%rsp)
	movq	%rdx, %rax
	jl	.LBB3_3
.LBB3_24:
	addq	$200, %rsp
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
.Lfunc_end3:
	.size	_Z11srad_tpal_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f, .Lfunc_end3-_Z11srad_tpal_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.cfi_endproc

	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3
.LCPI4_0:
	.quad	4598175219545276416
	.text
	.globl	_Z17srad_tpal_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.p2align	4, 0x90
	.type	_Z17srad_tpal_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f,@function
_Z17srad_tpal_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f:
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
	subq	$136, %rsp
	.cfi_def_cfa_offset 192
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	cmpl	%r9d, %r8d
	movl	%ecx, 28(%rsp)
	movl	%edx, 20(%rsp)
	movl	%esi, 16(%rsp)
	movl	%edi, 24(%rsp)
	jge	.LBB4_20
	vmovdqa	%xmm0, %xmm6
	vcvtss2sd	%xmm1, %xmm1, %xmm0
	vmulsd	.LCPI4_0(%rip), %xmm0, %xmm7
	movl	20(%rsp), %eax
	movl	%r8d, %ebp
	movq	280(%rsp), %rbx
	movq	272(%rsp), %r11
	movq	256(%rsp), %r15
	movq	248(%rsp), %r10
	movq	240(%rsp), %rdx
	movq	232(%rsp), %rcx
	movq	224(%rsp), %r8
	movq	216(%rsp), %r12
	vmovapd	%xmm1, %xmm5
	movl	%r9d, 12(%rsp)
	movl	%eax, %r13d
	cltq
	imull	16(%rsp), %r13d
	movq	%rax, 56(%rsp)
	vbroadcastsd	%xmm7, %ymm9
	vmovd	%r13d, %xmm0
	vpbroadcastd	%xmm0, %ymm8
	.p2align	4, 0x90
.LBB4_2:
	leal	64(%rbp), %edi
	cmpl	%r9d, %edi
	cmovgl	%r9d, %edi
	cmpl	%edi, %ebp
	jge	.LBB4_17
	movq	56(%rsp), %rax
	movslq	%edi, %r14
	movq	%rdx, %rsi
	movslq	%ebp, %rdx
	movl	%edi, 4(%rsp)
	movl	(%r11,%rax,4), %eax
	imull	16(%rsp), %eax
	movl	%eax, 8(%rsp)
	movq	%r14, %rax
	subq	%rdx, %rax
	cmpq	$8, %rax
	jb	.LBB4_13
	movq	%rax, 32(%rsp)
	movq	%rdx, %rax
	leal	(%r13,%rbp), %r8d
	notq	%rax
	addq	%r14, %rax
	leal	(%r8,%rax), %edi
	cmpl	%r8d, %edi
	jl	.LBB4_12
	movq	%rax, %rdi
	shrq	$32, %rdi
	jne	.LBB4_12
	addl	8(%rsp), %ebp
	leal	(%rbp,%rax), %eax
	cmpl	%ebp, %eax
	jl	.LBB4_12
	testq	%rdi, %rdi
	jne	.LBB4_12
	movq	32(%rsp), %rdi
	leaq	(%rbx,%rdx,4), %rsi
	movl	%r8d, %r11d
	movl	%ebp, %r9d
	xorl	%ebx, %ebx
	andq	$-8, %rdi
	leaq	(%rdi,%rdx), %rax
	.p2align	4, 0x90
.LBB4_10:
	vpaddd	(%rsi,%rbx,4), %ymm8, %ymm1
	leal	(%r9,%rbx), %r8d
	movq	224(%rsp), %rcx
	leal	(%r11,%rbx), %edx
	addq	$8, %rbx
	movslq	%r8d, %rbp
	movslq	%edx, %rdx
	cmpq	%rbx, %rdi
	vmovups	(%r15,%rbp,4), %ymm0
	vextracti128	$1, %ymm1, %xmm2
	vpmovsxdq	%xmm1, %ymm1
	vmovq	%xmm1, %rbp
	vpmovsxdq	%xmm2, %ymm2
	vmovss	(%r15,%rbp,4), %xmm3
	vmovq	%xmm2, %rbp
	vmovss	(%r15,%rbp,4), %xmm4
	vpextrq	$1, %xmm2, %rbp
	vextracti128	$1, %ymm2, %xmm2
	vinsertps	$16, (%r15,%rbp,4), %xmm4, %xmm4
	vmovq	%xmm2, %rbp
	vinsertps	$32, (%r15,%rbp,4), %xmm4, %xmm4
	vpextrq	$1, %xmm2, %rbp
	vmovups	(%r15,%rdx,4), %ymm2
	vinsertps	$48, (%r15,%rbp,4), %xmm4, %xmm4
	vpextrq	$1, %xmm1, %rbp
	vextracti128	$1, %ymm1, %xmm1
	vinsertps	$16, (%r15,%rbp,4), %xmm3, %xmm3
	vmovq	%xmm1, %rbp
	vinsertps	$32, (%r15,%rbp,4), %xmm3, %xmm3
	vpextrq	$1, %xmm1, %rbp
	vmulps	(%rcx,%rdx,4), %ymm2, %ymm1
	movq	232(%rsp), %rcx
	vinsertps	$48, (%r15,%rbp,4), %xmm3, %xmm3
	vmulps	(%rcx,%rdx,4), %ymm0, %ymm0
	movq	240(%rsp), %rcx
	vaddps	%ymm0, %ymm1, %ymm0
	vmulps	(%rcx,%rdx,4), %ymm2, %ymm1
	vcvtps2pd	(%r12,%rdx,4), %ymm2
	vaddps	%ymm1, %ymm0, %ymm0
	vinsertf128	$1, %xmm4, %ymm3, %ymm1
	vmulps	(%r10,%rdx,4), %ymm1, %ymm1
	vaddps	%ymm1, %ymm0, %ymm0
	vcvtps2pd	16(%r12,%rdx,4), %ymm1
	vextractf128	$1, %ymm0, %xmm3
	vcvtps2pd	%xmm0, %ymm0
	vcvtps2pd	%xmm3, %ymm3
	vmulpd	%ymm0, %ymm9, %ymm0
	vmulpd	%ymm3, %ymm9, %ymm3
	vaddpd	%ymm2, %ymm0, %ymm0
	vaddpd	%ymm1, %ymm3, %ymm1
	vcvtpd2ps	%ymm0, %xmm0
	vcvtpd2ps	%ymm1, %xmm1
	vinsertf128	$1, %xmm1, %ymm0, %ymm0
	vmovupd	%ymm0, (%r12,%rdx,4)
	jne	.LBB4_10
	movl	12(%rsp), %r9d
	movq	280(%rsp), %rbx
	movq	272(%rsp), %r11
	movq	240(%rsp), %rsi
	movq	232(%rsp), %rcx
	movq	224(%rsp), %r8
	cmpq	%rdi, 32(%rsp)
	jne	.LBB4_14
	jmp	.LBB4_16
.LBB4_12:
	movq	224(%rsp), %r8
.LBB4_13:
	movq	%rdx, %rax
.LBB4_14:
	movl	8(%rsp), %ebp
	.p2align	4, 0x90
.LBB4_15:
	leal	(%r13,%rax), %edx
	leal	(%rbp,%rax), %edi
	movslq	%edx, %rdx
	movslq	%edi, %rdi
	vmovss	(%r15,%rdi,4), %xmm1
	vmovss	(%r15,%rdx,4), %xmm0
	movl	(%rbx,%rax,4), %edi
	incq	%rax
	vmulss	(%r8,%rdx,4), %xmm0, %xmm3
	vmulss	(%rcx,%rdx,4), %xmm1, %xmm1
	vmulss	(%rsi,%rdx,4), %xmm0, %xmm0
	addl	%r13d, %edi
	cmpq	%r14, %rax
	movslq	%edi, %rdi
	vmovss	(%r15,%rdi,4), %xmm2
	vaddss	%xmm1, %xmm3, %xmm1
	vaddss	%xmm0, %xmm1, %xmm0
	vmulss	(%r10,%rdx,4), %xmm2, %xmm1
	vaddss	%xmm1, %xmm0, %xmm0
	vmovss	(%r12,%rdx,4), %xmm1
	vcvtss2sd	%xmm1, %xmm1, %xmm1
	vcvtss2sd	%xmm0, %xmm0, %xmm0
	vmulsd	%xmm0, %xmm7, %xmm0
	vaddsd	%xmm1, %xmm0, %xmm0
	vcvtsd2ss	%xmm0, %xmm0, %xmm0
	vmovss	%xmm0, (%r12,%rdx,4)
	jl	.LBB4_15
.LBB4_16:
	movl	4(%rsp), %eax
	movq	%rsi, %rdx
	movl	%eax, %ebp
.LBB4_17:
	cmpl	%r9d, %ebp
	jge	.LBB4_20
	movq	heartbeat@GOTPCREL(%rip), %rax
	cmpl	$0, (%rax)
	je	.LBB4_2
	subq	$8, %rsp
	.cfi_adjust_cfa_offset 8
	movq	%rdx, %rbx
	movl	28(%rsp), %edx
	movl	36(%rsp), %ecx
	movl	20(%rsp), %r9d
	movl	32(%rsp), %edi
	movl	24(%rsp), %esi
	movl	%ebp, %r8d
	movq	%r10, %r14
	vmovdqa	%xmm6, %xmm0
	vmovapd	%xmm5, %xmm1
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r11
	.cfi_adjust_cfa_offset 8
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r15
	.cfi_adjust_cfa_offset 8
	pushq	%r10
	.cfi_adjust_cfa_offset 8
	pushq	%rbx
	.cfi_adjust_cfa_offset 8
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	pushq	%r12
	.cfi_adjust_cfa_offset 8
	pushq	296(%rsp)
	.cfi_adjust_cfa_offset 8
	movl	296(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	movl	296(%rsp), %eax
	pushq	%rax
	.cfi_adjust_cfa_offset 8
	vmovss	%xmm5, 120(%rsp)
	vmovd	%xmm6, 116(%rsp)
	vmovapd	%xmm7, 144(%rsp)
	vmovdqu	%ymm8, 208(%rsp)
	vmovupd	%ymm9, 176(%rsp)
	vzeroupper
	callq	_Z20srad_handler_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f@PLT
	movq	%rbx, %rdx
	vmovupd	176(%rsp), %ymm9
	vmovdqu	208(%rsp), %ymm8
	vmovapd	144(%rsp), %xmm7
	movq	336(%rsp), %r8
	movl	124(%rsp), %r9d
	movq	344(%rsp), %rcx
	movq	384(%rsp), %r11
	movq	392(%rsp), %rbx
	vmovd	116(%rsp), %xmm6
	vmovss	120(%rsp), %xmm5
	movq	%r14, %r10
	addq	$112, %rsp
	.cfi_adjust_cfa_offset -112
	testl	%eax, %eax
	je	.LBB4_2
.LBB4_20:
	addq	$136, %rsp
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
.Lfunc_end4:
	.size	_Z17srad_tpal_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f, .Lfunc_end4-_Z17srad_tpal_inner_2iiiiiiiiPfS_fS_S_S_S_S_PiS0_S0_S0_f
	.cfi_endproc


	.ident	"clang version 7.1.0 (tags/RELEASE_710/final)"
	.section	".note.GNU-stack","",@progbits
	.addrsig
	.addrsig_sym heartbeat
