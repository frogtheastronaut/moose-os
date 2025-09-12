/*
    MooseOS Paging System Implementation
    Based on OSdev.org Setting Up Paging tutorial
    Copyright (c) 2025 Ethan Zhang.
*/

#include "include/paging.h"

// Static page directory and first page table - aligned to 4KB
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// Global variables
page_directory_t *kernel_directory = (page_directory_t*)page_directory;
page_directory_t *current_directory = (page_directory_t*)page_directory;

// Assembly functions for CR3 register manipulation  
void load_page_directory(uint32_t* page_dir) {
    asm volatile("mov %0, %%cr3" : : "r"(page_dir));
}

void enable_paging_asm(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // Set PG bit (bit 31)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

void disable_paging_asm(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000; // Clear PG bit (bit 31)
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

// TLB management functions
void flush_tlb(void) {
    asm volatile("mov %%cr3, %%eax; mov %%eax, %%cr3" : : : "eax");
}

void flush_tlb_entry(uint32_t virtual_addr) {
    asm volatile("invlpg (%0)" : : "r"(virtual_addr) : "memory");
}

// Initialize paging following OSdev wiki tutorial
void paging_init(uint32_t memory_size) {
    // Step 1: Create a blank page directory
    // Set each entry to not present
    for (int i = 0; i < 1024; i++) {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i] = 0x00000002;
    }
    
    // Step 2: Create your first page table
    // We will fill all 1024 entries in the table, mapping 4 megabytes
    for (unsigned int i = 0; i < 1024; i++) {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes ;)
        // attributes: supervisor level, read/write, present.
        first_page_table[i] = (i * 0x1000) | 3;
    }
    
    // Step 3: Put the page table in the page directory
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((unsigned int)first_page_table) | 3;
    
    // Step 4: Enable paging
    // Load page directory into CR3
    load_page_directory(page_directory);
    
    // Enable paging by setting the paging bit in CR0
    enable_paging_asm();
}

void enable_paging(page_directory_t *dir) {
    current_directory = dir;
    load_page_directory((uint32_t*)dir);
    enable_paging_asm();
}

void disable_paging(void) {
    disable_paging_asm();
}

void switch_page_directory(page_directory_t *dir) {
    current_directory = dir;
    load_page_directory((uint32_t*)dir);
    flush_tlb();
}

// Simplified implementations for the remaining functions
page_directory_t *create_page_directory(void) {
    return kernel_directory; // For now, return the static one
}

void destroy_page_directory(page_directory_t *dir) {
    // No-op for static allocation
}

page_directory_t *clone_page_directory(page_directory_t *src) {
    return src; // For now, return the same directory
}

page_table_t *get_page_table(uint32_t virtual_addr, page_directory_t *dir) {
    uint32_t table_index = GET_TABLE_INDEX(virtual_addr);
    uint32_t *pd = (uint32_t*)dir;
    
    if (!(pd[table_index] & 0x1)) { // Not present
        return NULL;
    }
    
    return (page_table_t*)(pd[table_index] & ~0xFFF);
}

page_table_t *create_page_table(uint32_t virtual_addr, page_directory_t *dir) {
    // For now, only return the first page table
    if (GET_TABLE_INDEX(virtual_addr) == 0) {
        return (page_table_t*)first_page_table;
    }
    return NULL;
}

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags, page_directory_t *dir) {
    // Simplified implementation - only works for first 4MB
    if (virtual_addr >= 0x400000) {
        return false; // Beyond first page table
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    first_page_table[page_index] = (physical_addr & ~0xFFF) | (flags & 0xFFF);
    
    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true;
}

bool unmap_page(uint32_t virtual_addr, page_directory_t *dir) {
    if (virtual_addr >= 0x400000) {
        return false; // Beyond first page table
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    first_page_table[page_index] = 0x00000002; // Not present
    
    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true;
}

uint32_t get_physical_addr(uint32_t virtual_addr, page_directory_t *dir) {
    if (virtual_addr >= 0x400000) {
        return 0; // Beyond first page table
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    uint32_t page_entry = first_page_table[page_index];
    
    if (!(page_entry & 0x1)) { // Not present
        return 0;
    }
    
    uint32_t page_offset = virtual_addr & (PAGE_SIZE - 1);
    return (page_entry & ~0xFFF) | page_offset;
}

// Stub implementations for frame allocator (not needed for basic setup)
uint32_t alloc_frame(void) {
    return 0; // Not implemented
}

void free_frame(uint32_t frame_addr) {
    // Not implemented
}

bool is_frame_allocated(uint32_t frame_addr) {
    return true; // Assume all allocated
}

void identity_map_kernel(page_directory_t *dir) {
    // Already done in paging_init
}

void *kmalloc_aligned(uint32_t size) {
    return NULL; // Not implemented
}

void *kmalloc_phys(uint32_t size, uint32_t *phys_addr) {
    return NULL; // Not implemented
}

void kfree_aligned(void *ptr) {
    // Not implemented
}

// Page fault handling - basic implementation
void page_fault_handler(uint32_t error_code, uint32_t virtual_addr) {
    // For now, just halt on page fault
    // This is where you'd handle page faults in a real implementation
    asm volatile("hlt");
}

// Debug functions (stubs)
void dump_page_directory(page_directory_t *dir) {
    // Debug implementation would go here
}

void dump_page_table(page_table_t *table) {
    // Debug implementation would go here
}