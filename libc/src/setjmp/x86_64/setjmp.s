.global setjmp
.global _setjmp
.global __setjmp
.type setjmp, @function
.type _setjmp, @function
.type __setjmp, @function

#
# int setjmp(jmp_buf env)
#
setjmp:
_setjmp:
__setjmp:
    mov %rbx, (%rdi)
    mov %rbp, 8(%rdi)
    mov %r12, 16(%rdi)
    mov %r13, 24(%rdi)
    mov %r14, 32(%rdi)
    mov %r15, 40(%rdi)
    lea 8(%rsp), %rdx
    mov %rdx, 48(%rdi)
    mov (%rsp), %rdx
    mov %rdx, 56(%rdi)
    xor %eax, %eax
    ret
