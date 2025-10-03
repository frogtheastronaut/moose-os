/*
    MooseOS Interrupt request code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "irq/irq.h"

static inline void io_wait(void) {
    write_port(0x80, 0); // delay port
}

void irq_remap(void)
{
    uint8_t mask1 = inb(0x21); // save master mask
    uint8_t mask2 = inb(0xA1); // save slave mask

    // start init sequence (ICW1)
    write_port(0x20, 0x11);
    io_wait();
    write_port(0xA0, 0x11);
    io_wait();

    // set vector offsets (ICW2)
    write_port(0x21, 0x20);
    io_wait();
    write_port(0xA1, 0x28);
    io_wait();

    // tell master that there is a slave at IRQ2 (ICW3)
    write_port(0x21, 0x04);
    io_wait();
    // tell slave its cascade identity (ICW3)
    write_port(0xA1, 0x02);
    io_wait();

    // set 8086 mode (ICW4)
    write_port(0x21, 0x01);
    io_wait();
    write_port(0xA1, 0x01);
    io_wait();

    // restore masks
    write_port(0x21, mask1);
    write_port(0xA1, mask2);
}
