; a 64 bit OS built in NASM assembly language
bits 64
section .data
	vga:		db 0xb8000
	vga_end:	db 0xb8fa0
	vga_size:	dw 0xfa0
	vga_width:	dw 80
	vga_height:	dw 25
	vga_cursor:	dw 0
	vga_color:	db 0x07
section .bss
	vga_buffer:		resb 0xfa0

section .text
	global _start

_start:
	; clear the screen
	mov rax, vga_buffer
	mov rcx, vga_size
	mov rdi, 0
	rep stosb

	; print a string
	mov rax, vga_buffer
	mov rcx, 5
	mov rdi, 'Hello'
	rep stosb

	; print a string
	mov rax, vga_buffer + 80
	mov rcx, 5
	mov rdi, 'World'
	rep stosb

	; halt
	hlt
