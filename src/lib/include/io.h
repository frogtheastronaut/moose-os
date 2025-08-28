#ifndef IO_H_
#define IO_H_

#include <stdint.h>

typedef unsigned short uint16_t;
typedef short int16_t;

// read byte from port
static inline uint8_t inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

// write byte to port
static inline void outb(uint16_t port, uint8_t data) {
    asm volatile("outb %0, %1" : : "a"(data), "Nd"(port));
}

// disable interrupts
static inline void cli(void) {
    asm volatile("cli");
}

// enable interrupts
static inline void sti(void) {
    asm volatile("sti");
}

#endif
