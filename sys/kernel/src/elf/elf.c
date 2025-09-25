#include "elf.h"
#include "heap/malloc.h"
#include "string/string.h"

elf_hdr* get_elf_hdr(void* data) {
    return (elf_hdr*)data;
}

int validate_elf_hdr(elf_hdr* hdr) {
    if (hdr->e_ident[EI_MAG0] != ELFMAG0 || hdr->e_ident[EI_MAG1] != ELFMAG1 ||
        hdr->e_ident[EI_MAG2] != ELFMAG2 || hdr->e_ident[EI_MAG3] != ELFMAG3)
        return 0;
    if (hdr->e_ident[EI_CLASS] != ELFCLASS32)
        return 0;
    if (hdr->e_type != ET_EXEC && hdr->e_type != ET_DYN)
        return 0;
    return 1;
}

uint32_t load_elf(elf_hdr* hdr) {
    elf_phdr* phdrs = (elf_phdr*)((uint8_t*)hdr + hdr->e_phoff);
    size_t total_size = 0, max_align = 0;
    elf_dynamic* dynamic = NULL;

    // Find memory size and alignment
    for (int i = 0; i < hdr->e_phnum; i++) {
        elf_phdr* ph = &phdrs[i];
        if (ph->p_type == PT_LOAD) {
            size_t end = ph->p_vaddr + ph->p_memsz;
            if (end > total_size) total_size = end;
            if (ph->p_align > max_align) max_align = ph->p_align;
        }
    }

    // Allocate memory for the image
    uint8_t* raw = (uint8_t*)malloc(total_size + max_align);
    uint8_t* base = (uint8_t*)(((uintptr_t)raw + (max_align - 1)) & ~(max_align - 1));
    if (!base) return 0;

    // Load segments
    for (int i = 0; i < hdr->e_phnum; i++) {
        elf_phdr* ph = &phdrs[i];
        if (ph->p_type == PT_DYNAMIC)
            dynamic = (elf_dynamic*)((uint8_t*)hdr + ph->p_offset);
        if (ph->p_type == PT_LOAD) {
            uint8_t* dest = base + ph->p_vaddr;
            uint8_t* src  = (uint8_t*)hdr + ph->p_offset;
            memcpy(dest, src, ph->p_filesz);
            if (ph->p_memsz > ph->p_filesz)
                memset(dest + ph->p_filesz, 0, ph->p_memsz - ph->p_filesz);
        }
    }

    // Parse dynamic section
    if (dynamic) {
        uint32_t strtab = 0, symtab = 0, rela = 0, relasz = 0, relaent = 0;
        while (dynamic->d_tag != DT_NULL) {
            switch(dynamic->d_tag) {
                case DT_STRTAB: strtab = dynamic->d_un.d_ptr; break;
                case DT_SYMTAB: symtab = dynamic->d_un.d_ptr; break;
                case DT_RELA:   rela   = dynamic->d_un.d_ptr; break;
                case DT_RELASZ: relasz = dynamic->d_un.d_val; break;
                case DT_RELAENT: relaent = dynamic->d_un.d_val; break;
                // Add more as needed
            }
            dynamic++;
        }
        apply_relocations(base, rela, relasz, relaent, symtab, strtab);
    }

    return (uint32_t)(base + hdr->e_entry);
}

void apply_relocations(uint8_t* base, uint32_t rela_vaddr, uint32_t relasz, uint32_t relaent,
                      uint32_t symtab_vaddr, uint32_t strtab_vaddr) {
    elf_relocation_addend* rela_table = (elf_relocation_addend*)(base + rela_vaddr);
    size_t rela_count = relasz / relaent;
    elf_symbol* symtab = (elf_symbol*)(base + symtab_vaddr);
    char* strtab = (char*)(base + strtab_vaddr);

    for (size_t i = 0; i < rela_count; i++) {
        elf_relocation_addend* rela = &rela_table[i];
        uint32_t symIndex = ELF32_R_SYM(rela->r_info);
        uint32_t type = ELF32_R_TYPE(rela->r_info);
        elf_symbol* sym = &symtab[symIndex];
        uint32_t* reloc_addr = (uint32_t*)(base + rela->r_offset);

        switch (type) {
            case R_386_32:
                *reloc_addr = sym->st_value + rela->r_addend;
                break;
            case R_386_RELATIVE:
                *reloc_addr = (uint32_t)(base + rela->r_addend);
                break;
            // Add more relocation types as needed
            default:
                // Print unknown relocation type
                break;
        }
    }
}