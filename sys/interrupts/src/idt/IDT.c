/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    ========================= OS THEORY =========================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.
    
    WHAT ARE INTERRUPTS?
    Interrupts are signals from a device, such as a keyboard or a hard drive, to the CPU, 
    telling it to immediately stop whatever it is currently doing and do something else.

    For example, a keyboard controller can send an interrupt when a character key was pressed. 
    Then the OS can display the character on screen immediately, even if the CPU was doing something 
    completely unrelated before, and return to what it was doing afterwards.

    When a specific interrupt arises, the CPU looks up an entry for that specific interrupt from a 
    table provided by the OS. In x86 protected mode the table is called the Interrupt Descriptor Table 
    (IDT) and can have up to 256 entries, but the name of this table and the maximum number of entries it 
    can have can differ based on the CPU architecture.

    After the CPU finds the entry for the interrupt, it jumps to the code the entry points to. 
    This code that is run in response to the interrupt is known as a interrupt service routine (ISR) 
    or an interrupt handler.

    There are generally 3 types of interrupts:
    1. Exceptions: These are interrupts that arise from the CPU itself, 
       usually due to some error or special condition. For example, a division by zero error 
       will raise an exception.
    2. Hardware Interrupts: These are interrupts that arise from hardware devices, 
       such as keyboards, mice, and hard drives. For example, a keyboard controller can raise
       an interrupt when a key is pressed.
    3. Software Interrupts: These are interrupts that are raised by software, to get the kernel's attention.

    Note: IDT initialization also requires a Global Descriptor Table (GDT) to be set up first.
    The GDT implementation can be found in ./GDT.c

    Source: https://wiki.osdev.org/Interrupts
            https://wiki.osdev.org/Interrupt_Descriptor_Table
*/

#include "idt/IDT.h"
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

