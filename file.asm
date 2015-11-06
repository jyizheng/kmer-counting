; BareMetal compile:
; nasm file.asm -o file.app


[BITS 64]
[ORG 0x0000000000200000]

%INCLUDE "bmdev.asm"

start:					; Start of program label

	mov rsi, file_name		; Load RSI with memory address of string
	call [b_output]			; Print the string that RSI points to

	mov rcx, 7
	call [b_mem_allocate]
	cmp rcx, 0
	je mem_failed
	mov rdx, rax

	call [b_file_open]
	cmp rax, 0
	je open_failed

	mov rcx, 13299136
	mov rdi, rdx
	call [b_file_read]
	call [b_file_close]

	mov rsi, rdi
	call [b_output]	
	jmp ret_os

open_failed:
	mov rsi, open_fail_msg
	call [b_output]
	jmp ret_os

mem_failed:
	mov rsi, mem_fail_msg
	call [b_output]
ret_os:
	ret					; Return to OS

file_name: db 'data.txt',0
open_fail_msg: db 'openfailed',0
mem_fail_msg: db 'memfailed',0
