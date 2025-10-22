.global longjmp
.global _longjmp
.type longjmp, @function
.type _longjmp, @function

#
# void longjmp(jmp_buf env, int val)
#
longjmp:
_longjmp:
	xor %eax, %eax
	cmp $1, %esi
	adc %esi, %eax
	mov (%rdi), %rbx
	mov 8(%rdi), %rbp
	mov 16(%rdi), %r12
	mov 24(%rdi), %r13
	mov 32(%rdi), %r14
	mov 40(%rdi), %r15
	mov 48(%rdi), %rsp
	jmp *56(%rdi)
