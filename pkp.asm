	.file	"pkp.c"
	.text
	.section	.rodata
.LC0:
	.string	"Hello, world!"
.LC1:
	.string	"it took: %ld\n"
	.text
	.globl	main
	.type	main, @function
main:
.LFB16:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	subq	$112, %rsp
	leaq	.LC0(%rip), %rax
	movq	%rax, %rdi
	call	puts@PLT
	movq	$0, -48(%rbp)
	movq	$4096, -40(%rbp)
	movb	$65, -105(%rbp)
	movq	-48(%rbp), %rax
	movq	%rax, -32(%rbp)
	movq	-40(%rbp), %rax
	movq	%rax, -24(%rbp)
	movq	-24(%rbp), %rsi
	movq	-32(%rbp), %rax
	movl	$0, %r9d
	movl	$-1, %r8d
	movl	$34, %ecx
	movl	$3, %edx
	movq	%rax, %rdi
	call	mmap@PLT
	movq	%rax, -16(%rbp)
	cmpq	$-1, -16(%rbp)
	jne	.L2
	movl	$0, %eax
	jmp	.L3
.L2:
	movq	-16(%rbp), %rax
.L3:
	movq	%rax, -8(%rbp)
	cmpq	$0, -8(%rbp)
	je	.L4
	movzbl	-105(%rbp), %ecx
	movq	-40(%rbp), %rdx
	movq	-8(%rbp), %rax
	movl	%ecx, %esi
	movq	%rax, %rdi
	call	memset@PLT
.L4:
	movq	-8(%rbp), %rax
	movq	%rax, -96(%rbp)
#APP
# 50 "pkp.c" 1
	mfence
	lfence
	rdtsc
	shl $32, %rdx
	or %rdx, %rax
	lfence
# 0 "" 2
#NO_APP
	movq	%rax, -56(%rbp)
	movq	-56(%rbp), %rax
	nop
	movq	%rax, -104(%rbp)
#APP
# 50 "pkp.c" 1
	mfence
	lfence
	rdtsc
	shl $32, %rdx
	or %rdx, %rax
	lfence
# 0 "" 2
#NO_APP
	movq	%rax, -64(%rbp)
	movq	-64(%rbp), %rax
	movq	%rax, -88(%rbp)
	movq	-96(%rbp), %rax
#APP
# 141 "pkp.c" 1
	mov (%rax), %al

# 0 "" 2
#NO_APP
	movb	%al, -106(%rbp)
#APP
# 67 "pkp.c" 1
	rdtscp
	shl $32, %rdx
	or %rdx, %rax
	lfence
# 0 "" 2
#NO_APP
	movq	%rax, -72(%rbp)
	movq	-72(%rbp), %rax
	movq	%rax, -80(%rbp)
	movq	-80(%rbp), %rax
	subq	-88(%rbp), %rax
	movq	%rax, %rsi
	leaq	.LC1(%rip), %rax
	movq	%rax, %rdi
	movl	$0, %eax
	call	printf@PLT
	movl	$0, %eax
	leave
	.cfi_def_cfa 7, 8
	ret
	.cfi_endproc
.LFE16:
	.size	main, .-main
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.2) 11.4.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
