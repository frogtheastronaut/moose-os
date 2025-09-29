/*
    MooseOS Paging System
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <stdbool.h>
#include "libc/lib.h"

// page size constants
#define PAGE_SIZE           4096
#define PAGE_ENTRIES        1024
#define PAGE_DIRECTORY_SIZE 1024

// page table entry flags
#define PAGE_PRESENT    0x001   // page is present in memory
#define PAGE_WRITABLE   0x002   // page is writable
#define PAGE_USER       0x004   // page is accessible from user mode
#define PAGE_ACCESSED   0x020   // page has been accessed
#define PAGE_DIRTY      0x040   // page has been written to

// memory layout
#define KERNEL_START        0x00100000  // 1MB - where kernel is loaded
#define KERNEL_END          0x00400000  // 4MB - end of kernel space
#define USER_START          0x40000000  // 1GB - start of user space
#define PAGE_TABLE_START    0x00400000  // 4MB - where page tables begin

// macros
#define PAGE_ALIGN_DOWN(addr)   ((addr) & ~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(addr)     (((addr) + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1))
#define GET_PAGE_INDEX(addr)    (((addr) >> 12) & 0x3FF)
#define GET_TABLE_INDEX(addr)   ((addr) >> 22)
#define MAKE_PHYS_ADDR(table, page) ((table << 22) | (page << 12))

// type definitions
typedef uint32_t page_table_t[PAGE_ENTRIES];
typedef uint32_t page_directory_t[PAGE_DIRECTORY_SIZE];

// global variables
extern page_directory_t *kernel_directory;
extern page_directory_t *current_directory;

void paging_init(uint32_t memory_size);
void enable_paging(page_directory_t *dir);
void disable_paging(void);
void switch_page_directory(page_directory_t *dir);

// page dir management
page_directory_t *create_page_directory(void);
void destroy_page_directory(page_directory_t *dir);
page_directory_t *clone_page_directory(page_directory_t *src);

// page table management  
page_table_t *get_page_table(uint32_t virtual_addr, page_directory_t *dir);
page_table_t *create_page_table(uint32_t virtual_addr, page_directory_t *dir);

// virtual memory mapping
bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags, page_directory_t *dir);
bool unmap_page(uint32_t virtual_addr, page_directory_t *dir);
uint32_t get_physical_addr(uint32_t virtual_addr, page_directory_t *dir);

// frame allocation
uint32_t alloc_frame(void);
void free_frame(uint32_t frame_addr);
bool is_frame_allocated(uint32_t frame_addr);

// memory allocation with paging
void *kmalloc_aligned(uint32_t size);
void *kmalloc_phys(uint32_t size, uint32_t *phys_addr);
void kfree_aligned(void *ptr);

// TLB management
void flush_tlb(void);
void flush_tlb_entry(uint32_t virtual_addr);

// page fault handling
void page_fault_handler(uint32_t error_code, uint32_t virtual_addr);

// identity mapping for kernel
void identity_map_kernel(page_directory_t *dir);


#endif // PAGING_H