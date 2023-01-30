global start

section .text
bits 32
start:
    ; Point the first entry of the level 4 page table to the first entry in the
    ; p3 table
    mov eax, p3_table
    or eax, 0b11
    mov dword [p4_table + 0], eax
	
	; Point the first entry of the level 3 page table to the first entry in the
    ; p2 table
    mov eax, p2_table
    or eax, 0b11
    mov dword [p3_table + 0], eax

	; point each page table level two entry to a page
    mov ecx, 0         ; counter variable
	.map_p2_table:
    mov eax, 0x200000  ; 2MiB
    mul ecx
    or eax, 0b10000011
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table

    ; move page table address to cr3
    mov eax, p4_table
    mov cr3, eax

	; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

	; set the long mode bit
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

	; enable paging
    mov eax, cr0
    or eax, 1 << 31
    or eax, 1 << 16
    mov cr0, eax

    lgdt [gdt64.pointer]

	; update selectors
	mov ax, gdt64.data
	mov ss, ax
	mov ds, ax
	mov es, ax

	; jump to long mode!
	jmp gdt64.code:long_mode_start

section .bss

align 4096

p4_table:
	resb 4096
p3_table:
	resb 4096
p2_table:
	resb 4096

section .rodata
gdt64:
    dq 0
.code: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41) | (1<<43) | (1<<53)
.data: equ $ - gdt64
    dq (1<<44) | (1<<47) | (1<<41)
.pointer:
    dw .pointer - gdt64 - 1
    dq gdt64

section .text
bits 64
global _keyboard_handler
global _read_port
global _write_port
global _load_idt
extern kmain 		;this is defined in the c file
extern keyboard_handler_main

_read_port:
	mov edx, [esp + 4]
			;al is the lower 8 bits of eax
	in al, dx	;dx is the lower 16 bits of edx
	ret

_write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

_load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti 				;turn on interrupts
	ret

_keyboard_handler:                 
	call    keyboard_handler_main
	iretd


long_mode_start:

	; clear screen
	mov rcx, 0
	.clear_screen:
	mov qword [0xb8000 + rcx], 0x0000000000000000
	add rcx, 8
	cmp rcx, 80 * 25 * 2
	jne .clear_screen

	cli 				;block interrupts
	mov esp, stack_space
	call kmain
	hlt 				;halt the CPU

	;mov rax, 0x2f6c2f652f722f67 ; grel	
	;mov [0xb8000], rax
	;mov rax, 0x2f202f532f4f2f6c ; lOS 
	;mov [0xb8000+8], rax
	;mov rax, 0x2f592f412f4b2f4f ; OKAY
	;mov [0xb8000+16], rax


	; clear screen
	;mov rcx, 0
	;.clear_screen2:
	;mov qword [0xb8000 + rcx], 0x0000000000000000
	;add rcx, 8
	;cmp rcx, 80 * 25 * 2
	;jne .clear_screen2

	;cli ; disable interrupts
	;hlt

stack_space:

	
