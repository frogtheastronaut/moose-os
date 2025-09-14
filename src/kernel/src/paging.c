/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/
  
/*
    ============================== OS THEORY ==============================

    Paging is a system which allows each process to see a full virtual address space, 
    without actually requiring the full amount of physical memory to be available or present.
    This is good because if you don't have paging, two programs might be using the same
    physical memory addresses, causing them to interfere with each other and leading to crashes or data corruption
    
    Paging is achieved through the use of the Memory Management Unit (MMU). On the x86, the MMU 
    maps memory through a series of tables, two to be exact. They are the paging directory (PD), 
    and the paging table (PT).

    Both tables contain 1024 4-byte entries, making them 4 KiB each. In the page directory, each 
    entry points to a page table. In the page table, each entry points to a 4 KiB physical page frame.
    Additionally, each entry has bits controlling access protection and caching features of the structure 
    to which it points. The entire system consisting of a page directory and page tables represents a linear 
    4-GiB virtual memory map.
    
    MEMORY LAYOUT IN MooseOS:
    - 0x00000000 - 0x00400000: Kernel space (4MB) - identity mapped
    - 0x00400000+: Page tables and allocated memory
    - 0x40000000+: User space (starts at 1GB)

    Source: https://wiki.osdev.org/Paging
    
*/

#include "../include/paging.h"

// Static page directory and first page table - 4kb
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

// Global variables
page_directory_t *kernel_directory = (page_directory_t*)page_directory;
page_directory_t *current_directory = (page_directory_t*)page_directory;

// Frame allocator globals
static uint32_t next_frame = 0x00500000; // Start after kernel (5MB)
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

/**
 * Initialise paging
 * The following code is largely credited to OSDev wiki
 */
void paging_init(uint32_t memory_size) {
    // Create a blank page directory
    for (int i = 0; i < 1024; i++) {
        // This sets the following flags to the pages:
        //   Supervisor: Only kernel-mode can access them
        //   Write Enabled: It can be both read from and written to
        //   Not Present: The page table is not present
        page_directory[i] = 0x00000002;
    }

    // Create first page table
    // We will fill all 1024 entries in the table, mapping 4 megabytes
    for (unsigned int i = 0; i < 1024; i++) {
        // As the address is page aligned, it will always leave 12 bits zeroed.
        // Those bits are used by the attributes
        // attributes: supervisor level, read/write, present.
        first_page_table[i] = (i * 0x1000) | 3;
    }
    
    // Put the page table in the page directory
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((unsigned int)first_page_table) | 3;
    
    // Enable paging
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

// Frame allocator - simple implementation
uint32_t alloc_frame(void) {
    uint32_t frame = next_frame;
    next_frame += PAGE_SIZE;
    frames_allocated++;
    return frame;
}

void free_frame(uint32_t frame_addr) {
    // Simple implementation - just decrement counter
    // In a real OS, you'd maintain a free list of frames
    if (frames_allocated > 0) {
        frames_allocated--;
    }
}

bool is_frame_allocated(uint32_t frame_addr) {
    // Check if frame is within allocated range
    return (frame_addr >= 0x00500000 && frame_addr < next_frame);
}

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

// Page fault handling - basic implementation
void page_fault_handler(uint32_t error_code, uint32_t virtual_addr) {
    // For now, just halt on page fault
    // This is where you'd handle page faults in a real implementation
    asm volatile("hlt");
}

// Debug functions - basic implementations
void dump_page_directory(page_directory_t *dir) {
    if (!dir) return;
    
    // In a real implementation, you'd print to console/serial
    // For now, this is just a placeholder that walks the directory
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if ((*dir)[i] & PAGE_PRESENT) {
            // Directory entry is present
            // You could print: "Page table %d: 0x%x\n", i, (*dir)[i]
        }
    }
}

void dump_page_table(page_table_t *table) {
    if (!table) return;
    
    // Walk through page table entries
    for (int i = 0; i < PAGE_ENTRIES; i++) {
        if ((*table)[i] & PAGE_PRESENT) {
            // Page is present
            // You could print: "Page %d: 0x%x\n", i, (*table)[i]
        }
    }
}