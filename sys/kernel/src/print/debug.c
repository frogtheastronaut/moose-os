/*
    MooseOS Mouse interrupt handler code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "print/debug.h"

// check if the serial port is ready to transmit
static int serial_is_transmit_empty(void) {
    return inb(0x3F8 + 5) & 0x20;
}

// write a single character to serial
void serial_write_char(char c) {
    while (!serial_is_transmit_empty());
    outb(0x3F8, (uint8_t)c);
}

// write a null-terminated string to serial
void debugf(const char* str) {
	if (!detect_qemu()) {
		return; // Only output if running in QEMU
	}
    for (size_t i = 0; str[i]; i++) {
        serial_write_char(str[i]);
    }
}