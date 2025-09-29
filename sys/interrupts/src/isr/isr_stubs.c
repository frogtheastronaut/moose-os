/*
    MooseOS Interrupt Service Routine stubs
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#include "isr/isr.h"

// array of ISR stubs
void (*const isr_stubs[ISR_EXCEPTION_AMOUNT])(void) = {
    _isr0,  _isr1,  _isr2,  _isr3,  _isr4,  _isr5,  _isr6,  _isr7,
    _isr8,  _isr9,  _isr10, _isr11, _isr12, _isr13, _isr14, _isr15,
    _isr16, _isr17, _isr18, _isr19, _isr20, _isr21, _isr22, _isr23,
    _isr24, _isr25, _isr26, _isr27, _isr28, _isr29, _isr30, _isr31
};
