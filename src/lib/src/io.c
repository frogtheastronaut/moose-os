/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "../include/io.h"

// Read byte from port
uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Write byte to port
void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// Read word (16-bit) from port
uint16_t inw(uint16_t port) {
    uint16_t result;
    asm volatile("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// Write word (16-bit) to port
void outw(uint16_t port, uint16_t data) {
    asm volatile("outw %0, %1" : : "a"(data), "Nd"(port));
}

// Disable interrupts
void cli(void) {
    asm volatile("cli");
}

// Enable interrupts
void sti(void) {
    asm volatile("sti");
}