/*
    MooseOS IRQ code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef IRQ_H
#define IRQ_H

#include "idt/idt.h"
void irq_remap(void);

#endif // IRQ_H