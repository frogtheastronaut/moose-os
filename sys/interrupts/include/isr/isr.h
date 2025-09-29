/*
    MooseOS ISR code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef ISR_H
#define ISR_H

#include "idt/idt.h"

#define ISR_EXCEPTION_AMOUNT 32

// stubs
extern void (*const isr_stubs[ISR_EXCEPTION_AMOUNT])(void);

// handler
extern void isr_handler(void* stack);

// error messages
extern const char* exception_messages[ISR_EXCEPTION_AMOUNT];

// ISR initialisation code
extern void isr_init(void);

// ISRs
extern void _isr0();
extern void _isr1();
extern void _isr2();
extern void _isr3();
extern void _isr4();
extern void _isr5();
extern void _isr6();
extern void _isr7();
extern void _isr8();
extern void _isr9();
extern void _isr10();
extern void _isr11();
extern void _isr12();
extern void _isr13();
extern void _isr14();
extern void _isr15();
extern void _isr16();
extern void _isr17();
extern void _isr18();
extern void _isr19();
extern void _isr20();
extern void _isr21();
extern void _isr22();
extern void _isr23();
extern void _isr24();
extern void _isr25();
extern void _isr26();
extern void _isr27();
extern void _isr28();
extern void _isr29();
extern void _isr30();
extern void _isr31();

#endif // ISR_H