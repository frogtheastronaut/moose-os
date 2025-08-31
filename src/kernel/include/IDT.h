/*
	MooseOS
	Copyright (c) 2025 Ethan Zhang
*/
#ifndef IDT_H
#define IDT_H

#include "keyhandler.h"
#include "lock.h"
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

extern volatile bool key_pressed;
extern volatile char last_keycode;


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
extern struct GDT_entry GDT[3];
extern struct GDT_ptr gdt_ptr;
// idt entry
extern struct IDT_entry IDT[IDT_SIZE];

void idt_init(void);
void kb_init(void);

#endif