/*
    MooseOS I/O port operations
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef IO_H_
#define IO_H_

#include <stdint.h>

typedef unsigned short uint16_t;
typedef short int16_t;

/**
 * @todo add inl and outl. we dont need them yet
 */
uint8_t inb(uint16_t port);
void outb(uint16_t port, uint8_t data);
uint16_t inw(uint16_t port);
void outw(uint16_t port, uint16_t data);

#endif // IO_H_
