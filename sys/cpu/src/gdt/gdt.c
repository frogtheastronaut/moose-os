/*
    MooseOS GDT System
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#include "gdt/gdt.h"

// GDT entries and pointer
struct GDT_entry GDT[6];
struct GDT_ptr gdt_ptr;

// TSS
TSS_entry tss;

/**
 * encode a GDT entry   
 * @param num entry number in the GDT
 * @param base base address of the segment
 * @param limit segment limit
 * @param access access byte (permissions)
 * @param gran granularity and flags
 */
void gdt_encode(int num, unsigned long base, unsigned long limit, unsigned char access, unsigned char gran) {
    GDT[num].base_low = (base & 0xFFFF);
    GDT[num].base_middle = (base >> 16) & 0xFF;
    GDT[num].base_high = (base >> 24) & 0xFF;
    GDT[num].limit_low = (limit & 0xFFFF);
    GDT[num].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
    GDT[num].access = access;
}

/**
 * initialise the GDT and TSS
 */
void gdt_init(void) {
    gdt_ptr.limit = (sizeof(struct GDT_entry) * 6) - 1;
    gdt_ptr.base = (unsigned int)(uintptr_t)&GDT;

    gdt_encode(0, 0, 0, 0, 0);                // null segment
    gdt_encode(1, 0, 0xFFFFFFFF, 0x9A, 0xCF); // code segment
    gdt_encode(2, 0, 0xFFFFFFFF, 0x92, 0xCF); // data segment

    gdt_encode(3, 0, 0xFFFFF, 0xFA, 0xCF);
    gdt_encode(4, 0, 0xFFFFF, 0xF2, 0xCF);

    // set the TSS
    uint32_t tss_base = (uint32_t)&tss;
    gdt_encode(5, tss_base, sizeof(tss) - 1, 0x89, 0x00);

    gdt_flush(get_physical_addr((uint32_t) &gdt_ptr, kernel_directory));

    tss.ss0  = 0x10; 
    tss.iomap = sizeof(tss);

    tss_flush(0x28);

    gdt_flush((unsigned int)(uintptr_t)&gdt_ptr);
}