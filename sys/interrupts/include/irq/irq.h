/*
    MooseOS IRQ code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#ifndef IRQ_H
#define IRQ_H

#include "idt/idt.h"
void irq_remap(void);

#endif // IRQ_H