.intel_syntax noprefix

.section .note.GNU-stack,"",%progbits

.section .text
	.global _start

# Defined here, and not in syscalls.S
__syscall_exit:
	mov rax, 60
	# rdi filled by C
	syscall

_start:
	call main
	mov rdi, rax
	call __syscall_exit
