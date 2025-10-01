/**
    MooseOS Paging System
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details

    This paging system is worthy of a potato.
    @todo implement proper paging systems
*/

#include "paging/paging.h"
#include "print/debug.h"

// page directory and first page table
uint32_t page_directory[1024] __attribute__((aligned(4096)));
uint32_t first_page_table[1024] __attribute__((aligned(4096)));

page_directory_t *kernel_directory = (page_directory_t*)page_directory;
page_directory_t *current_directory = (page_directory_t*)page_directory;

// frame variables
static uint32_t next_frame = 0x00500000;
static uint32_t frames_allocated = 0;

// assembly functions for CR3 register manipulation
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
    // create a blank page directory
    for (int i = 0; i < 1024; i++) {
        page_directory[i] = 0x00000002;
    }

    // create first page table
    for (unsigned int i = 0; i < 1024; i++) {
        // attributes: supervisor level, read/write, present.
        first_page_table[i] = (i * 0x1000) | 3;
    }
    
    // attributes: supervisor level, read/write, present
    page_directory[0] = ((unsigned int)first_page_table) | 3;
    
    // load page directory into CR3
    load_page_directory(page_directory);

    // enable paging by setting the paging bit in CR0
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
    // allocate aligned memory for the new page directory
    page_directory_t *new_dir = (page_directory_t*)kmalloc_aligned(sizeof(page_directory_t));
    if (!new_dir) {
        debugf("Failed to allocate memory for new page directory\n");
        return NULL;
    }
    
    // initialize all entries as not present
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        (*new_dir)[i] = 0x00000002; // supervisor, writable, not present
    }
    
    // copy kernel mappings (first 4MB) from kernel directory
    (*new_dir)[0] = (*kernel_directory)[0];
    
    return new_dir;
}

void destroy_page_directory(page_directory_t *dir) {
    if (!dir || dir == kernel_directory) {
        debugf("Attempted to destroy invalid or kernel page directory\n");
        return; // don't destroy kernel directory or NULL
    }
    
    // free all page tables (except kernel ones)
    for (int i = 1; i < PAGE_DIRECTORY_SIZE; i++) {
        if ((*dir)[i] & PAGE_PRESENT) {
            // free the page table
            page_table_t *table = (page_table_t*)((*dir)[i] & ~0xFFF);
            kfree_aligned(table);
        }
    }
    
    // free the directory itself
    kfree_aligned(dir);
}

/** @note unused */
page_directory_t *clone_page_directory(page_directory_t *src) {
    if (!src) {
        debugf("No page directory to clone\n");
        return NULL;
    }
    
    // create new page directory
    page_directory_t *new_dir = create_page_directory();
    if (!new_dir) {
        debugf("Failed to create new page directory\n");
        return NULL;
    }

    // copy all entries from source
    for (int i = 0; i < PAGE_DIRECTORY_SIZE; i++) {
        if ((*src)[i] & PAGE_PRESENT) {
            if (i == 0) {
                // kernel space - share the same page table
                (*new_dir)[i] = (*src)[i];
            } else {
                // user space - clone the page table
                page_table_t *src_table = (page_table_t*)((*src)[i] & ~0xFFF);
                page_table_t *new_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t));
                
                if (!new_table) {
                    destroy_page_directory(new_dir);
                    return NULL;
                }
                
                // copy all page entries
                for (int j = 0; j < PAGE_ENTRIES; j++) {
                    (*new_table)[j] = (*src_table)[j];
                }
                
                // set directory entry with same flags
                (*new_dir)[i] = ((uint32_t)new_table) | ((*src)[i] & 0xFFF);
            }
        }
    }
    
    return new_dir;
}

page_table_t *get_page_table(uint32_t virtual_addr, page_directory_t *dir) {
    uint32_t table_index = GET_TABLE_INDEX(virtual_addr);
    uint32_t *pd = (uint32_t*)dir;
    
    if (!(pd[table_index] & 0x1)) { // not present
        debugf("Page table for virtual address does not exist\n");
        return NULL;
    }
    
    return (page_table_t*)(pd[table_index] & ~0xFFF);
}

page_table_t *create_page_table(uint32_t virtual_addr, page_directory_t *dir) {
    uint32_t table_index = GET_TABLE_INDEX(virtual_addr);
    uint32_t *pd = (uint32_t*)dir;
    
    // check if page table already exists
    if (pd[table_index] & PAGE_PRESENT) {
        return (page_table_t*)(pd[table_index] & ~0xFFF);
    }
    
    // for kernel space (first 4MB), return existing table
    if (table_index == 0) {
        return (page_table_t*)first_page_table;
    }
    
    // allocate new page table for user space
    page_table_t *new_table = (page_table_t*)kmalloc_aligned(sizeof(page_table_t));
    if (!new_table) {
        return NULL;
    }
    
    // initialize all entries as not present
    for (int i = 0; i < PAGE_ENTRIES; i++) {
        (*new_table)[i] = 0x00000002; // supervisor, writable, not present
    }
    
    // add page table to directory
    pd[table_index] = ((uint32_t)new_table) | PAGE_PRESENT | PAGE_WRITABLE;
    
    return new_table;
}

bool map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags, page_directory_t *dir) {
    // get or create the page table for this virtual address
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) {
        table = create_page_table(virtual_addr, dir);
        if (!table) { // could not create page table
            debugf("Failed to create page table for mapping\n");
            return false; 
        }
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    (*table)[page_index] = (physical_addr & ~0xFFF) | (flags & 0xFFF);
    
    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true; // success
}

/** @note unused */
bool unmap_page(uint32_t virtual_addr, page_directory_t *dir) {
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) { // page table doesn't exist
        debugf("Page table for virtual address does not exist\n");
        return false;
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    (*table)[page_index] = 0x00000002;

    if (dir == current_directory) {
        flush_tlb_entry(virtual_addr);
    }
    
    return true;
}

/** @note unused */
uint32_t get_physical_addr(uint32_t virtual_addr, page_directory_t *dir) {
    page_table_t *table = get_page_table(virtual_addr, dir);
    if (!table) {
        return 0; // page table doesn't exist
    }
    
    uint32_t page_index = GET_PAGE_INDEX(virtual_addr);
    uint32_t page_entry = (*table)[page_index];
    
    if (!(page_entry & PAGE_PRESENT)) {
        return 0; // page not present
    }
    
    uint32_t page_offset = virtual_addr & (PAGE_SIZE - 1);
    return (page_entry & ~0xFFF) | page_offset;
}

/** frame allocator. @note unused */
uint32_t alloc_frame(void) {
    uint32_t frame = next_frame;
    next_frame += PAGE_SIZE;
    frames_allocated++;
    return frame;
}

/** @note unused */
void free_frame(uint32_t frame_addr) {
    // just decrements counter
    /** @todo Implement frame deallocation */
    if (frames_allocated > 0) {
        frames_allocated--;
    }
}

/** @note unused */
bool is_frame_allocated(uint32_t frame_addr) {
    // check if frame is within allocated range
    return (frame_addr >= 0x00500000 && frame_addr < next_frame);
}

/** @note unused */
void identity_map_kernel(page_directory_t *dir) {
    // identity map the kernel space (first 4MB) with kernel privileges
    for (uint32_t addr = 0; addr < KERNEL_END; addr += PAGE_SIZE) {
        map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE, dir);
    }
}

void *kmalloc_aligned(uint32_t size) {
    // align size to page boundary
    size = PAGE_ALIGN_UP(size);
    
    // allocate from frame allocator
    uint32_t frame = next_frame;
    next_frame += size;
    frames_allocated++;
    
    return (void*)frame;
}

/** @note unused */
void *kmalloc_phys(uint32_t size, uint32_t *phys_addr) {
    void *virt_addr = kmalloc_aligned(size);
    if (phys_addr && virt_addr) {
        *phys_addr = (uint32_t)virt_addr; // identity mapped for now
    }
    return virt_addr;
}

void kfree_aligned(void *ptr) {
    // just decrease the counter for now
    frames_allocated--;
}
