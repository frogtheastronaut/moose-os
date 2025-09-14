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

#include "../include/IDT.h"
#include "../include/GDT.h"
#include "../include/paging.h"

volatile bool key_pressed = false;
volatile char last_keycode = 0;

// IDT entry table
struct IDT_entry IDT[IDT_SIZE];

/**
 * Initialize the Interrupt Descriptor Table
 */
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

    /* idt entry for page fault interrupt, exception 14 */
    unsigned long page_fault_address = (unsigned long)page_fault_handler_asm;
    IDT[14].offset_lowerbits = page_fault_address & 0xffff;
    IDT[14].selector = KERNEL_CODE_SEGMENT_OFFSET;
    IDT[14].zero = 0;
    IDT[14].type_attr = INTERRUPT_GATE;
    IDT[14].offset_higherbits = (page_fault_address & 0xffff0000) >> 16;

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
    if (handler_lock != 0) return;
    handler_lock = 1;

    unsigned char status;
    char keycode;

    /* write EOI */
    write_port(0x20, 0x20);


    status = read_port(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        keycode = read_port(KEYBOARD_DATA_PORT);
        last_keycode = keycode;
        key_pressed = true;
    }

    handler_lock = 0;
}
