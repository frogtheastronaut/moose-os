/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    Header file for ../src/GDT.c
    
*/

#ifndef GDT_H
#define GDT_H

#include <stdint.h>

// GDT entry structure
struct GDT_entry {
    unsigned short limit_low;
    unsigned short base_low;
    unsigned char base_middle;
    unsigned char access;
    unsigned char granularity;
    unsigned char base_high;
} __attribute__((packed));

// GDT pointer structure
struct GDT_ptr {
    unsigned short limit;
    unsigned int base;
} __attribute__((packed));

// External functions
extern void load_gdt(unsigned int gdt_ptr);

// Function prototypes
void gdt_encode(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);
void gdt_init(void);

#endif