/*
    Moose Operating System - main script setting up IDT and GDT
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/keyhandler.h"

// defines
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

// load gdt & idt
extern void load_gdt(unsigned int);
extern void load_idt(unsigned long *idt_ptr);

// handlers
extern void keyboard_handler(void);
extern void mouse_handler(void);
extern void timer_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);

unsigned int current_loc = 0;
/* video memory begins here */
char *vidptr = (char*)0xb8000;

struct GDT_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));
struct GDT_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));
// IDT entry structure
struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};


// gdts
struct GDT_entry GDT[3];
struct GDT_ptr gdt_ptr;
// idt entry
struct IDT_entry IDT[IDT_SIZE];

// encode
void gdt_encode(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
    GDT[num].base_low = (base & 0xFFFF);
    GDT[num].base_middle = (base >> 16) & 0xFF;
    GDT[num].base_high = (base >> 24) & 0xFF;
    GDT[num].limit_low = (limit & 0xFFFF);
    GDT[num].granularity = (limit >> 16) & 0x0F;
    GDT[num].granularity |= gran & 0xF0;
    GDT[num].access = access;
}

// init gdt
void gdt_init() {
    gdt_ptr.limit = (sizeof(struct GDT_entry) * 3) - 1;
    gdt_ptr.base = (unsigned int)&GDT;

    gdt_encode(0, 0, 0, 0, 0);                // Null 
    gdt_encode(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // Code 
    gdt_encode(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // Data

    load_gdt((unsigned int)&gdt_ptr);
}

// init, innit?
void idt_init(void)
{
    unsigned long keyboard_address;
    unsigned long mouse_address;
    unsigned long timer_address;
    unsigned long idt_address;
    unsigned long idt_ptr[2];
    caps = false;

    /* init our gdt */
    gdt_init();

    /* populate IDT entry of timer interrupt (IRQ0) */
    timer_address = (unsigned long)timer_handler;
    IDT[0x20].offset_lowerbits = timer_address & 0xffff;
    IDT[0x20].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x20].zero = 0;
    IDT[0x20].type_attr = INTERRUPT_GATE;
    IDT[0x20].offset_higherbits = (timer_address & 0xffff0000) >> 16;

    /* populate IDT entry of keyboard interrupt */
    keyboard_address = (unsigned long)keyboard_handler;
    IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
    IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x21].zero = 0;
    IDT[0x21].type_attr = INTERRUPT_GATE;
    IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16;

    /* idt entry for mouse interrupt, IRQ12 */
    mouse_address = (unsigned long)mouse_handler;
    IDT[0x2C].offset_lowerbits = mouse_address & 0xffff;
    IDT[0x2C].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[0x2C].zero = 0;
    IDT[0x2C].type_attr = INTERRUPT_GATE;
    IDT[0x2C].offset_higherbits = (mouse_address & 0xffff0000) >> 16;

	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);

	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	write_port(0x21 , 0xF9);
	write_port(0xA1 , 0xEF);  
}

void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		// goes to keyhandler.c 
		processKey(keyboard_map_normal[(unsigned char) keycode], keycode);
	}
}