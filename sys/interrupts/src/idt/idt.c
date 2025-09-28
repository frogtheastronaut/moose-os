/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "idt/idt.h"
#include "irq/irq.h"

volatile bool key_pressed = false;
volatile char last_keycode = 0;

struct idt_descriptor_t idt_descriptor;

// IDT entry table
struct IDT_entry IDT[IDT_SIZE];

void idt_set_entry(unsigned char num, unsigned long base, unsigned short selector, unsigned char flags)
{
    IDT[num].offset_lowerbits = base & 0xFFFF;
    IDT[num].selector = selector;
    IDT[num].zero = 0;
    IDT[num].type_attr = flags;
    IDT[num].offset_higherbits = (base >> 16) & 0xFFFF;
}

static void initialise_all_entries(void)
{
    unsigned long keyboard_address;
    unsigned long mouse_address;
    unsigned long timer_address;

    /* IDT entry of timer interrupt (IRQ0) */
    timer_address = (unsigned long)timer_handler;
    idt_set_entry(0x20, timer_address, KERNEL_CODE_SEGMENT_OFFSET, INTERRUPT_GATE);

    /* IDT entry of keyboard interrupt (IRQ1) */
    keyboard_address = (unsigned long)keyboard_handler;
    idt_set_entry(0x21, keyboard_address, KERNEL_CODE_SEGMENT_OFFSET, INTERRUPT_GATE);

    /* IDT entry of mouse interrupt (IRQ12) */
    mouse_address = (unsigned long)mouse_handler;
    idt_set_entry(0x2C, mouse_address, KERNEL_CODE_SEGMENT_OFFSET, INTERRUPT_GATE);

    /* IDT entry of page fault interrupt (exception 14) */
    unsigned long page_fault_address = (unsigned long)page_fault_handler_asm;
    idt_set_entry(14, page_fault_address, KERNEL_CODE_SEGMENT_OFFSET, INTERRUPT_GATE);
}
/**
 * Initialize the Interrupt Descriptor Table
 */
void idt_init(void)
{
    gdt_init();
    initialise_all_entries();
    irq_remap();

    idt_descriptor.limit = sizeof(struct IDT_entry) * IDT_SIZE - 1;
    idt_descriptor.base = (unsigned int)IDT;

    idt_load(&idt_descriptor);
}

