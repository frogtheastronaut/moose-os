/*
	MooseOS
	Copyright (c) 2025 Ethan Zhang
*/
#ifndef IDT_H
#define IDT_H

#include "../../drivers/include/keyhandler.h"
#include "../../kernel/include/lock.h"
#include "../../gdt/include/GDT.h"
#include "../../kernel/include/paging.h"

// defines
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x08

// load gdt & idt
extern void gdt_flush(unsigned int);
extern void idt_load(void* idt_descriptor);
extern void gdt_init(void);


// handlers
extern void keyboard_handler(void);
extern void mouse_handler(void);
extern void timer_handler(void);
extern void page_fault_handler_asm(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void pic_remap(void);


// IDT entry structure
struct IDT_entry {
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct idt_descriptor_t {
	unsigned short limit;
	unsigned int base;
} __attribute__((packed));

extern struct idt_descriptor_t idt_descriptor;
extern struct IDT_entry IDT[IDT_SIZE];

void idt_init(void);
void kb_init(void);

#endif