#include "keyboard.h"
#include <stdint.h>

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

#define ENTER_KEY_CODE 0x1C
#define BACK_KEY_CODE 0x0E
#define SPACE_KEY_CODE 0x39

extern unsigned char keyboard_map[128];
extern void _keyboard_handler(void);
extern char _read_port(unsigned short port);
extern void _write_port(unsigned short port, unsigned char data);
extern void _load_idt(unsigned long *idt_ptr);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

/* current color */
unsigned char current_color = 0x0F;

char command = ' ';


struct IDT_entry {
   uint16_t offset_lowerbits;        // offset bits 0..15
   uint16_t selector;        // a code segment selector in GDT or LDT
   uint8_t  ist;             // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
   uint8_t  type_attributes; // gate type, dpl, and p fields
   uint16_t offset_midbits;        // offset bits 16..31
   uint32_t offset_higherbits;        // offset bits 32..63
   uint32_t zero;            // reserved
};

struct IDT_entry IDT[IDT_SIZE];


void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)_keyboard_handler;
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
	IDT[0x21].zero = 0;
	IDT[0x21].type_attributes = INTERRUPT_GATE;
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	/* ICW1 - begin initialization */
	_write_port(0x20 , 0x11);
	_write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT */
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	_write_port(0x21 , 0x20);
	_write_port(0xA1 , 0x28);

	/* ICW3 - setup cascading */
	_write_port(0x21 , 0x00);
	_write_port(0xA1 , 0x00);

	/* ICW4 - environment info */
	_write_port(0x21 , 0x01);
	_write_port(0xA1 , 0x01);
	/* Initialization finished */

	/* mask interrupts */
	_write_port(0x21 , 0xff);
	_write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	_load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	_write_port(0x21 , 0xFD);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		vidptr[current_loc++] = 0x62;
	}
}

void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x62;
	}
}


void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;
	/* write EOI */
	_write_port(0x20, 0x20);

	status = _read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = _read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0){
			return;
        }
        if(keycode == ENTER_KEY_CODE){
            kprint_newline();
            return;
        }
        else{
            char letter = keyboard_map[(unsigned char)keycode];
            vidptr[current_loc++] = letter;
            vidptr[current_loc++] = 0x62;
        }

    }   
}


void kmain(void)
{
	const char *str = "It's Grellin' Time";
	clear_screen();
	kprint(str);
	kprint_newline();
	kprint_newline();

	idt_init();
	kb_init();

	while(1);
}