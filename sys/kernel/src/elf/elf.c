#include "elf.h"
#include "heap/malloc.h"
#include "string/string.h"
#include "paging/paging.h"
#include "print/debug.h"

// ELF loading constants
#define ELF_LOAD_BASE 0x08048000  // Standard ELF load address
#define MAX_ELF_SIZE  0x10000000  // 256MB max ELF size
#define MAX_SEGMENTS  64          // Max program segments

elf_hdr* get_elf_hdr(void* data) {
    return (elf_hdr*)data;
}

int validate_elf_hdr(elf_hdr* hdr) {
    // Validate ELF magic numbers
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3) {
        debugf("[ELF] Invalid magic numbers\n");
        return 0;
    }
    
    // Check for 32-bit architecture
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32) {
        debugf("[ELF] Not a 32-bit ELF\n");
        return 0;
    }
    
    // Check ELF type (executable or shared object)
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN) {
        debugf("[ELF] Invalid ELF type\n");
        return 0;
    }
    
    // Validate program header bounds
    if (hdr->e_phnum > MAX_SEGMENTS) {
        debugf("[ELF] Too many program segments\n");
        return 0;
    }
    
    // Check for reasonable entry point
    if (hdr->e_entry == 0 || hdr->e_entry < ELF_LOAD_BASE) {
        debugf("[ELF] Invalid entry point\n");
        return 0;
    }
    
    // Validate header size
    if (hdr->e_ehsize < sizeof(elf_hdr)) {
        debugf("[ELF] Invalid header size\n");
        return 0;
    }
    
    return 1;
}

uint32_t load_elf(elf_hdr* hdr) {
    return load_elf_with_paging(hdr, current_directory);
}

uint32_t load_elf_with_paging(elf_hdr* hdr, page_directory_t* page_dir) {
    if (!hdr || !page_dir) {
        debugf("[ELF] Invalid parameters\n");
        return 0;
    }

    elf_phdr* phdrs = (elf_phdr*)((uint8_t*)hdr + hdr->e_phoff);
    elf_dynamic* dynamic = NULL;
    
    // Validate program headers are within bounds
    if (hdr->e_phoff + (hdr->e_phnum * sizeof(elf_phdr)) > MAX_ELF_SIZE) {
        debugf("[ELF] Program headers out of bounds\n");
        return 0;
    }

    // First pass: validate all segments
    for (int i = 0; i < hdr->e_phnum; i++) {
        elf_phdr* ph = &phdrs[i];
        
        if (ph->p_type == PT_LOAD) {
            // Validate segment bounds
            if (ph->p_vaddr < ELF_LOAD_BASE || 
                ph->p_vaddr + ph->p_memsz > ELF_LOAD_BASE + MAX_ELF_SIZE) {
                debugf("[ELF] Segment virtual address out of bounds\n");
                return 0;
            }
            
            // Check for integer overflow
            if (ph->p_vaddr + ph->p_memsz < ph->p_vaddr) {
                debugf("[ELF] Segment size overflow\n");
                return 0;
            }
            
            // Validate file offset
            if (ph->p_offset + ph->p_filesz < ph->p_offset) {
                debugf("[ELF] File offset overflow\n");
                return 0;
            }
        }
    }

    // Second pass: load segments with proper virtual memory mapping
    for (int i = 0; i < hdr->e_phnum; i++) {
        elf_phdr* ph = &phdrs[i];
        
        if (ph->p_type == PT_DYNAMIC) {
            dynamic = (elf_dynamic*)((uint8_t*)hdr + ph->p_offset);
        }
        
        if (ph->p_type == PT_LOAD) {
            // Calculate memory permissions
            uint32_t flags = PAGE_PRESENT | PAGE_USER;
            if (ph->p_flags & 0x2) flags |= PAGE_WRITABLE; // PF_W
            
            // Map pages for this segment
            uint32_t start_page = PAGE_ALIGN_DOWN(ph->p_vaddr);
            uint32_t end_page = PAGE_ALIGN_UP(ph->p_vaddr + ph->p_memsz);
            
            for (uint32_t vaddr = start_page; vaddr < end_page; vaddr += PAGE_SIZE) {
                uint32_t paddr = alloc_frame();
                if (!paddr) {
                    debugf("[ELF] Failed to allocate physical frame\n");
                    return 0;
                }
                
                if (!map_page(vaddr, paddr, flags, page_dir)) {
                    debugf("[ELF] Failed to map page\n");
                    free_frame(paddr);
                    return 0;
                }
            }
            
            // Copy segment data page by page to avoid large memory operations
            uint8_t* src = (uint8_t*)hdr + ph->p_offset;
            
            // Validate source offset is within reasonable bounds
            if (ph->p_offset >= MAX_ELF_SIZE || ph->p_filesz > MAX_ELF_SIZE) {
                debugf("[ELF] Invalid file offset or size\n");
                return 0;
            }
            
            debugf("[ELF] Copying segment data\n");
            
            // Copy data page by page with proper page directory switching
            page_directory_t* old_dir = current_directory;
            uint32_t bytes_copied = 0;
            
            for (uint32_t vaddr = start_page; vaddr < end_page; vaddr += PAGE_SIZE) {
                // Switch to target page directory to access the page
                if (page_dir != current_directory) {
                    switch_page_directory(page_dir);
                }
                
                uint8_t* page_dest = (uint8_t*)vaddr;
                uint32_t page_offset = 0;
                uint32_t copy_size = PAGE_SIZE;
                
                // Calculate offset within the first page
                if (vaddr == start_page) {
                    page_offset = ph->p_vaddr - start_page;
                    page_dest += page_offset;
                    copy_size -= page_offset;
                }
                
                // Adjust copy size for last page and available data
                uint32_t remaining_file_data = ph->p_filesz - bytes_copied;
                if (copy_size > remaining_file_data) {
                    copy_size = remaining_file_data;
                }
                
                // Copy file data if available
                if (copy_size > 0 && bytes_copied < ph->p_filesz) {
                    memcpy(page_dest, src + bytes_copied, copy_size);
                    bytes_copied += copy_size;
                }
                
                // Zero out remaining space in this page (BSS section)
                uint32_t remaining_mem = ph->p_memsz - bytes_copied;
                uint32_t zero_size = PAGE_SIZE - page_offset - copy_size;
                if (zero_size > 0 && remaining_mem > 0) {
                    if (zero_size > remaining_mem) zero_size = remaining_mem;
                    memset(page_dest + copy_size, 0, zero_size);
                }
                
                // Switch back to original page directory
                if (page_dir != old_dir) {
                    switch_page_directory(old_dir);
                }
            }
            
            debugf("[ELF] Segment copied successfully\n");
        }
    }

    // Handle dynamic linking if present
    if (dynamic) {
        if (!process_dynamic_section(dynamic, page_dir)) {
            debugf("[ELF] Failed to process dynamic section\n");
            return 0;
        }
    }

    return hdr->e_entry;
}

int process_dynamic_section(elf_dynamic* dynamic, page_directory_t* page_dir) {
    uint32_t strtab = 0, symtab = 0, rela = 0, relasz = 0, relaent = 0;
    
    // Parse dynamic section entries
    while (dynamic->d_tag != DT_NULL) {
        switch(dynamic->d_tag) {
            case DT_STRTAB: strtab = dynamic->d_un.d_ptr; break;
            case DT_SYMTAB: symtab = dynamic->d_un.d_ptr; break;
            case DT_RELA:   rela   = dynamic->d_un.d_ptr; break;
            case DT_RELASZ: relasz = dynamic->d_un.d_val; break;
            case DT_RELAENT: relaent = dynamic->d_un.d_val; break;
            // Add more dynamic tags as needed
            default:
                break;
        }
        dynamic++;
    }
    
    // Apply relocations if present
    if (rela && relasz && relaent) {
        return apply_relocations(rela, relasz, relaent, symtab, strtab, page_dir);
    }
    
    return 1; // Success
}

int apply_relocations(uint32_t rela_vaddr, uint32_t relasz, uint32_t relaent,
                     uint32_t symtab_vaddr, uint32_t strtab_vaddr, page_directory_t* page_dir) {
    if (relasz == 0 || relaent == 0) return 1; // No relocations
    
    elf_relocation_addend* rela_table = (elf_relocation_addend*)rela_vaddr;
    size_t rela_count = relasz / relaent;
    elf_symbol* symtab = (elf_symbol*)symtab_vaddr;
    
    // Validate relocation count
    if (rela_count > MAX_SEGMENTS * 1000) { // Reasonable limit
        debugf("[ELF] Too many relocations\n");
        return 0;
    }

    for (size_t i = 0; i < rela_count; i++) {
        elf_relocation_addend* rela = &rela_table[i];
        uint32_t symIndex = ELF32_R_SYM(rela->r_info);
        uint32_t type = ELF32_R_TYPE(rela->r_info);
        uint32_t* reloc_addr = (uint32_t*)(rela->r_offset);
        
        // Validate relocation address is in valid range
        if (rela->r_offset < ELF_LOAD_BASE || 
            rela->r_offset >= ELF_LOAD_BASE + MAX_ELF_SIZE) {
            debugf("[ELF] Relocation address out of bounds\n");
            return 0;
        }

        switch (type) {
            case R_386_32:
                if (symtab && symIndex < 10000) { // Reasonable symbol limit
                    elf_symbol* sym = &symtab[symIndex];
                    *reloc_addr = sym->st_value + rela->r_addend;
                }
                break;
            case R_386_RELATIVE:
                *reloc_addr = (uint32_t)(ELF_LOAD_BASE + rela->r_addend);
                break;
            case R_386_PC32:
                if (symtab && symIndex < 10000) {
                    elf_symbol* sym = &symtab[symIndex];
                    *reloc_addr = sym->st_value + rela->r_addend - rela->r_offset;
                }
                break;
            default:
                debugf("[ELF] Unknown relocation type\n");
                break;
        }
    }
    
    return 1; // Success
}

void free_elf_memory(elf_hdr* hdr, page_directory_t* page_dir) {
    if (!hdr || !page_dir) return;
    
    elf_phdr* phdrs = (elf_phdr*)((uint8_t*)hdr + hdr->e_phoff);
    
    // Free all mapped pages for LOAD segments
    for (int i = 0; i < hdr->e_phnum; i++) {
        elf_phdr* ph = &phdrs[i];
        
        if (ph->p_type == PT_LOAD) {
            uint32_t start_page = PAGE_ALIGN_DOWN(ph->p_vaddr);
            uint32_t end_page = PAGE_ALIGN_UP(ph->p_vaddr + ph->p_memsz);
            
            for (uint32_t vaddr = start_page; vaddr < end_page; vaddr += PAGE_SIZE) {
                uint32_t paddr = get_physical_addr(vaddr, page_dir);
                if (paddr) {
                    unmap_page(vaddr, page_dir);
                    free_frame(paddr);
                }
            }
        }
    }
}