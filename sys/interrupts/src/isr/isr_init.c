/*
    MooseOS Interrupt Service Routine code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "isr/isr.h"
#include "idt/idt.h"
#include <stddef.h>
#include <stdint.h>

// list of error messages
/**
 * @todo move them into a header file
 */
const char *exception_messages[] = {
    "Division By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Into Detected Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bad TSS",
    "Segment Not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception ",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

// initialise all exception ISRs
void isr_init(void) {
    for (size_t i = 0; i < ISR_EXCEPTION_AMOUNT; i++) {
        idt_set_entry(
            i,
            (unsigned long) isr_stubs[i],
            KERNEL_CODE_SEGMENT_OFFSET,
            INTERRUPT_GATE
        );
    }
}