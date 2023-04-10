.intel_syntax noprefix

.section .note.GNU-stack,"",%progbits

# Registers on entry:
# rax  system call number
# rcx  return address
# r11  saved rflags (note: r11 is callee-clobbered register in C ABI)
# rdi  arg0
# rsi  arg1
# rdx  arg2
# r10  arg3 (needs to be moved from rcx to conform to C ABI)
# r8   arg4
# r9   arg5

.section .text
	.global _start

__syscall_read:
	mov rax, 0
	# fd           is in rdi
	# char *buf    is in rsi
	# size_t count is in rdx
	syscall
	ret

__syscall_write:
	mov rax, 1
	# fd           is in rdi
	# char *buf    is in rsi
	# size_t count is in rdx
	syscall
	ret

__syscall_open:
	mov rax, 3
	# char *fname  is in rdi
	# int flags    is in rsi
	# umode_t mode is in rdx
	syscall
	ret

__syscall_exit:
	mov rax, 60
	# rdi filled by C
	syscall

_start:
	call main
	mov rdi, rax
	call __syscall_exit
