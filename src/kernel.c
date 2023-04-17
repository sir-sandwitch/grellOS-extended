/* compiled with -ffreestanding in gcc */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "include/keyboard.h"
#include "include/io.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

/* the VGA framebuffer starts at 0xb8000 */
#define VIDEO_MEMORY 0xb8000

#define GDT_OFFSET_KERNEL_CODE asm("gdt64")


#define KBD_DATA_PORT 0x60

/* defines an IDT entry */
typedef struct {
	uint16_t    isr_low;      // The lower 16 bits of the ISR's address
	uint16_t    kernel_cs;    // The GDT segment selector that the CPU will load into CS before calling the ISR
	uint8_t	    ist;          // The IST in the TSS that the CPU will load into RSP; set to zero for now
	uint8_t     attributes;   // Type and attributes; see the IDT page
	uint16_t    isr_mid;      // The higher 16 bits of the lower 32 bits of the ISR's address
	uint32_t    isr_high;     // The higher 32 bits of the ISR's address
	uint32_t    reserved;     // Set to zero
} __attribute__((packed)) idt_entry_t;

/* define idt */
__attribute__((aligned(0x10))) 
static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance

/* idtr struct */
typedef struct {
	uint16_t	limit;
	uint64_t	base;
} __attribute__((packed)) idtr_t;

/* define idtr */
static idtr_t idtr;

/* define isr stub table */

/* general exception handler */
__attribute__((noreturn))
void exception_handler(void);
void exception_handler() {
    __asm__ volatile ("cli; hlt"); // Completely hangs the computer
}



/* idt helper function */
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags) {
    idt_desc_t* descriptor = &idt[vector];
 
    descriptor->base_low       = (uint64_t)isr & 0xFFFF;
    descriptor->cs             = GDT_OFFSET_KERNEL_CODE;
    descriptor->ist            = 0;
    descriptor->attributes     = flags;
    descriptor->base_mid       = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->base_high      = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->rsv0           = 0;
}

/* idt init function */
extern void* isr_stub_table[];
 
void idt_init(void);
void idt_init() {
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;
 
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }
 
    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag
}

/* this function writes a single character out to the screen */
void kprint(char *c, int line, int column, int color) {
    /* the VGA framebuffer consists of 25 lines each of 80 columns; each element takes 2 bytes */
    char *vidmem = (char *)VIDEO_MEMORY;
    int offset = BYTES_FOR_EACH_ELEMENT * (COLUMNS_IN_LINE * line + column);
    vidmem[offset] = *c;
    vidmem[offset + 1] = color; /* color is an attribute byte */
}

/* this function clears the screen */
void kclear() {
    char *vidmem = (char *)VIDEO_MEMORY;
    int i;
    for (i = 0; i < SCREENSIZE; i++) {
        vidmem[i] = 0;
    }
}

/* this function prints a string to the screen */
void kprint_string(char *s, int startLine, int startColumn, int color) {
    int i = 0;
    while (s[i] != '\0') {
        kprint(&s[i], startLine, startColumn + i, color);
        i++;
    }
}

void kmain() {
    init_keyboard();
    kprint_string("Welcome to ", 3, 0, 0x0f);
    kprint_string("grellOS", 3, 11, 0x0d);
}
