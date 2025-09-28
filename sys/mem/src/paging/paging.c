/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "paging/paging.h"

uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// Global variables
page_directory_t *kernel_directory = (page_directory_t*)page_directory;
page_directory_t *current_directory = (page_directory_t*)page_directory;

// Frame allocator globals
static uint32_t next_frame = 0x00500000;
static uint32_t frames_allocated = 0;

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

void paging_init(uint32_t memory_size) {
    // Create a blank page directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // Create first page table
    for (unsigned int i = 0; i < 1024; i++) {
        // attributes: supervisor level, read/write, present.
        first_page_table[i] = (i * 0x1000) | 3;
    }
    
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((unsigned int)first_page_table) | 3;
    
    // Load page directory into CR3
    load_page_directory(page_directory);
    
    // Enable paging by setting the paging bit in CR0
    enable_paging_asm();
}

/**
 * @note enable_paging and disable_paging are not used.
 */
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

page_directory_t *create_page_directory(void) {
    // Allocate aligned memory for the new page directory
    page_directory_t *new_dir = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t));
    if (!new_dir) {
        return NULL;
    }
    
    // Initialize all entries as not present
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        (*new_dir)[i] = 0x00000002; // Supervisor, writable, not present
    }
    
    // Copy kernel mappings (first 4MB) from kernel directory
    (*new_dir)[0] = (*kernel_directory)[0];
    
    return new_dir;
}

void destroy_page_directory(page_directory_t *dir) {
    if (!dir || dir == kernel_directory) {
        return; // Don't destroy kernel directory or NULL
    }
    
    // Free all page tables (except kernel ones)
    for (int i = 1; i < PAGE_DIRECTORY_SIZE; i++) {
        if ((*dir)[i] & PAGE_PRESENT) {
            // Free the page table
            page_table_t *table = (page_table_t*)((*dir)[i] & ~0xFFF);
            kfree_aligned(table);
        }
    }
    
    // Free the directory itself
    kfree_aligned(dir);
}

/** @note unused */
page_directory_t *clone_page_directory(page_directory_t *src) {
    if (!src) {
        return NULL;
    }
    
    // Create new page directory
    page_directory_t *new_dir = create_page_directory();
    if (!new_dir) {
        return NULL;
    }
    
    // Copy all entries from source
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if ((*src)[i] & PAGE_PRESENT) {
            if (i == 0) {
                // Kernel space - share the same page table
                (*new_dir)[i] = (*src)[i];
            } else {
                // User space - clone the page table
                page_table_t *src_table = (page_table_t*)((*src)[i] & ~0xFFF);
                page_table_t *new_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t));
                
                if (!new_table) {
                    destroy_page_directory(new_dir);
                    return NULL;
                }
                
                // Copy all page entries
                for (int j = 0; j < PAGE_ENTRIES; j++) {
                    (*new_table)[j] = (*src_table)[j];
                }
                
                // Set directory entry with same flags
                (*new_dir)[i] = ((uint32_t)new_table) | ((*src)[i] & 0xFFF);
            }
        }
    }
    
    return new_dir;
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
    uint32_t table_index = GET_TABLE_INDEX(virtual_addr);
    uint32_t *pd = (uint32_t*)dir;
    
    // Check if page table already exists
    if (pd[table_index] & PAGE_PRESENT) {
        return (page_table_t*)(pd[table_index] & ~0xFFF);
    }
    
    // For kernel space (first 4MB), return existing table
    if (table_index == 0) {
        return (page_table_t*)first_page_table;
    }
    
    // Allocate new page table for user space
    page_table_t *new_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t));
    if (!new_table) {
        return NULL;
    }
    
    // Initialize all entries as not present
    for (int i = 0; i < PAGE_ENTRIES; i++) {
        (*new_table)[i] = 0x00000002; // Supervisor, writable, not present
    }
    
    // Add page table to directory
    pd[table_index] = ((uint32_t)new_table) | PAGE_PRESENT | PAGE_WRITABLE;
    
    return new_table;
}

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags, page_directory_t *dir) {
    // Get or create the page table for this virtual address
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) {
        table = create_page_table(virtual_addr, dir);
        if (!table) {
            return false; // Could not create page table
        }
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    (*table)[page_index] = (physical_addr & ~0xFFF) | (flags & 0xFFF);
    
    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true;
}

/** @note unused */
bool unmap_page(uint32_t virtual_addr, page_directory_t *dir) {
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) {
        return false; // Page table doesn't exist
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    (*table)[page_index] = 0x00000002; // Not present
    
    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true;
}

/** @note unused */
uint32_t get_physical_addr(uint32_t virtual_addr, page_directory_t *dir) {
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) {
        return 0; // Page table doesn't exist
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    uint32_t page_entry = (*table)[page_index];
    
    if (!(page_entry & PAGE_PRESENT)) {
        return 0; // Page not present
    }
    
    uint32_t page_offset = virtual_addr & (PAGE_SIZE - 1);
    return (page_entry & ~0xFFF) | page_offset;
}

/** Frame allocator. @note unused */
uint32_t alloc_frame(void) {
    uint32_t frame = next_frame;
    next_frame += PAGE_SIZE;
    frames_allocated++;
    return frame;
}

/** @note unused */
void free_frame(uint32_t frame_addr) {
    // Just decrements counter
    /** @todo Implement frame deallocation */
    if (frames_allocated > 0) {
        frames_allocated--;
    }
}

/** @note unused */
bool is_frame_allocated(uint32_t frame_addr) {
    // Check if frame is within allocated range
    return (frame_addr >= 0x00500000 && frame_addr < next_frame);
}

/** @note unused */
void identity_map_kernel(page_directory_t *dir) {
    // Identity map the kernel space (first 4MB) with kernel privileges
    for (uint32_t addr = 0; addr < KERNEL_END; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE, dir);
    }
}

void *kmalloc_aligned(uint32_t size) {
    // Align size to page boundary
    size = PAGE_ALIGN_UP(size);
    
    // For simplicity, allocate from our frame allocator
    uint32_t frame = next_frame;
    next_frame += size;
    frames_allocated++;
    
    return (void*)frame;
}

/** @note unused */
void *kmalloc_phys(uint32_t size, uint32_t *phys_addr) {
    void *virt_addr = kmalloc_aligned(size);
    if (phys_addr && virt_addr) {
        *phys_addr = (uint32_t)virt_addr; // Identity mapped for now
    }
    return virt_addr;
}

void kfree_aligned(void *ptr) {
    // For now, we don't track individual allocations
    // In a real implementation, you'd maintain a free list
    frames_allocated--;
}

void page_fault_handler(uint32_t error_code, uint32_t virtual_addr) {
    // Just halt on page fault
    asm volatile("hlt");
}

void page_fault_handler_main(uint32_t error_code) {

    // Get the virtual address that caused the page fault
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r" (faulting_address));

    // Call the main page fault handler
    page_fault_handler(error_code, faulting_address);

}