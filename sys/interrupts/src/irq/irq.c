/*
    MooseOS Interrupt Request code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "irq/irq.h"

void irq_remap(void)
{
	write_port(0x20 , 0x11); // initialise PIC1 and PIC2
	write_port(0xA0 , 0x11);

	write_port(0x21 , 0x20);
	write_port(0xA1 , 0x28);

	write_port(0x21 , 0x00);
	write_port(0xA1 , 0x00);

	write_port(0x21 , 0x01); // 8086 mode
	write_port(0xA1 , 0x01);

	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);
}