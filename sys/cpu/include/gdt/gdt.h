/*
    MooseOS GDT System
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include "paging/paging.h"

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

typedef struct TSS_entry {
    uint32_t prev;
    uint32_t esp0; uint32_t ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3, eip, eflags, eax, ecx, edx, ebx;
    uint32_t esp, ebp, esi, edi;
    uint16_t es, cs, ss, ds, fs, gs; // <-- changed to uint16_t
    uint32_t ldt;
    uint16_t trap, iomap;
} __attribute__((packed)) TSS_entry;

extern TSS_entry tss;

// external functions
// GDT flush defined  in gdt.asm
extern void gdt_flush(unsigned int gdt_ptr);
extern void tss_flush(unsigned short tss_segment_selector);

// function prototypes
void gdt_encode(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran);
void gdt_init(void);

#endif // GDT_H