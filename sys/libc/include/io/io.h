#ifndef IO_H_
#define IO_H_

#include <stdint.h>

typedef unsigned short uint16_t;
typedef short int16_t;

uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

#endif
