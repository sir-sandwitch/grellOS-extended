/* compiled with -ffreestanding in gcc, so std functions are limited. if something needs implementation, make a header in ./include/ */

#ifndef STDINT_INCLUDE
#include <stdint.h>
#define STDINT_INCLUDE
#endif
#ifndef STDDEF_INCLUDE
#include <stddef.h>
#define STDDEF_INCLUDE
#endif
#ifndef STDBOOL_INCLUDE
#include <stdbool.h>
#define STDBOOL_INCLUDE
#endif
#ifndef KEYBOARD_INCLUDE
#include "include/keyboard.h"
#define KEYBOARD_INCLUDE
#endif
#ifndef IO_INCLUDE
#include "include/io.h"
#define IO_INCLUDE
#endif
#ifndef IDT_INCLUDE
#include "include/idt.h"
#define IDT_INCLUDE
#endif
#ifndef TIMER_INCLUDE
#include "include/timer.h"
#define TIMER_INCLUDE
#endif
#ifndef APIC_INCLUDE
#include "include/apic.h"
#define APIC_INCLUDE
#endif

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

/* the VGA framebuffer starts at 0xb8000 */
#define VIDEO_MEMORY 0xb8000
extern void gdt4(void) {}
#define GDT_OFFSET_KERNEL_CODE &gdt4

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64

static bool vectors[IDT_MAX_DESCRIPTORS];

bool keyboard_enabled = false;

int cursor_pos_line = 0;
int cursor_pos_column = 0;

/* init pic */
void pic_init(void){
    /* send init command (0x11) */
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    /* remap pic past 0x20 */
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    /* enable cascading */
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    /* set 8086 mode */
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    /* mask all but keyboard interrupt */
    outb(0x21, 0xFD);
    outb(0xA1, 0xFF);
}

void keyboard_init(void){
    /* enable keyboard */
    outb(0x64, 0xAE);
    /* enable interrupts */
    outb(0x21, 0xFD);
    outb(0x20, 0x20);
    keyboard_enabled = true;
}

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

// /* idtr struct */
// typedef struct {
// 	uint16_t	limit;
// 	uint64_t	base;
// } __attribute__((packed)) idtr_t;

/* define idtr */
static idtr_t idtr;

/* general exception handler */
__attribute__((noreturn))
void exception_handler(void);
void exception_handler() {
    kprint_string("Fatal exception", 0, 0, 0x04);
    __asm__ volatile("cli");
    __asm__ volatile("hlt");
}



/* idt helper function */
//void idt_set_descriptor(uint8_t vector, void* isr, uint8_t flags);
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
    __asm__ volatile ("cli"); // clear the interrupt flag
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;
 
    for (uint8_t vector = 0; vector < 32; vector++) {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    /* timer */
    isr_stub_table[32] = &timer_handler;
    idt_set_descriptor(32, isr_stub_table[32], 0x8E);
    vectors[32] = true;

    /* cmos timer */
    isr_stub_table[40] = &cmos_handler;
    idt_set_descriptor(40, isr_stub_table[40], 0x8E);
    vectors[40] = true;

    /* keyboard */
    isr_stub_table[33] = &keyboard_handler;
    idt_set_descriptor(33, isr_stub_table[33], 0x8E);
    vectors[33] = true;
    if(!keyboard_enabled){
        keyboard_init();
    }
 
    __asm__ volatile ("lidt %0" : : "m"(idtr)); // load the new IDT
    __asm__ volatile ("sti"); // set the interrupt flag
}

/* handle timer interrupt */
void timer_handler() {
    outb(0x20, 0x20);
}

void cmos_handler() {
    outb(0x20, 0x20);
}


void keyboard_handler(void){
    unsigned char status;
    char keycode;

    /* write EOI */
    outb(0x20, 0x20);

    status = inb(KBD_STATUS_PORT);
    
    /* Lowest bit of status will be set if buffer is not empty */
    if (status & 0x01) {
        keycode = inb(KBD_DATA_PORT);
        if(keycode < 0){
            return;
        }
        kprint((unsigned char) keyboard_map[keycode], cursor_pos_line, cursor_pos_column, 0x0f);
        cursor_pos_column++;
        if (cursor_pos_column >= COLUMNS_IN_LINE) {
            cursor_pos_column = 0;
            cursor_pos_line++;
            if (cursor_pos_line >= LINES) {
                kclear();
                cursor_pos_line = 0;
            }
        }
    }
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
void kclear(void) {
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

void kmain(void) {
    if(check_apic()){
        kprint_string("APIC detected, disabling...", 0, 0, 0x0f);
        apic_disable();
    } else {
        kprint_string("APIC not detected, booting", 0, 0, 0x0f);
    }
    //idt_init();
   // pic_init();
    //keyboard_init();
    kprint_string("Welcome to ", 3, 0, 0x0f);
    kprint_string("grellOS", 3, 11, 0x0d);
}
