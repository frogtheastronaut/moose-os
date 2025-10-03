/*
    MooseOS ELF parser
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "elf/elf.h"
#include "heap/heap.h"
#include "print/debug.h"

#define READ_EXEC 0x5

static bool check_file(elf_header* header) {
    if (!header) {
        return false;
    }

    if (
        header->e_ident[EI_MAG0] != EI_MAG0_VALUE 
        || header->e_ident[EI_MAG1] != EI_MAG1_VALUE
        || header->e_ident[EI_MAG2] != EI_MAG2_VALUE 
        || header->e_ident[EI_MAG3] != EI_MAG3_VALUE
    ) {
        debugf("[ELF] ELF magic is incorrect\n");
        return false;
    }

    return true;
}

static bool check_file_support(elf_header* header) {
    if (!check_file(header)) {
        debugf("[ELF] Invalid ELF file\n");
        return false;
    }

    bool is_valid = true;
    if (header->e_ident[EI_CLASS] != ELFCLASS32) {
        debugf("[ELF] Unsupported ELF File Class\n");
        is_valid = false;
    }

    if (header->e_ident[EI_DATA] != ELFDATA2LSB) {
        debugf("[ELF] Unsupported ELF file byte order\n");
        is_valid = false;
    }

    if (header->e_machine != EM_386) {
        debugf("[ELF] Unsupported ELF file target\n");
        is_valid = false;
    }

    if (header->e_ident[EI_VERSION] != EV_CURRENT) {
        debugf("[ELF] Unsupported ELF File version\n");
        is_valid = false;
    }

    if (header->e_type != ET_REL && header->e_type != ET_EXEC) {
        debugf("[ELF] Unsupported ELF File type\n");
        is_valid = false;
    }

    return is_valid;
}

bool elf_parse(void* elf_data, elf_load_info* out_info) {
    elf_header* header = (elf_header*) elf_data;
    if (!check_file_support(header)) {
        return false;
    }

    size_t seg_count = 0;
    elf_segment* segments_arr = kmalloc(sizeof(elf_segment) * header->e_phnum);
    elf_phdr* phdrs = (elf_phdr*) ((uint8_t*) elf_data + header->e_phoff);

    for (size_t i = 0; i < header->e_phnum; i++) {
        elf_phdr* phdr = &phdrs[i];
        elf_segment segment;
        
        if (phdr->p_type != PT_LOAD) {
            continue;
        }

        segment.file_size = phdr->p_filesz;
        segment.flags = phdr->p_flags;
        segment.mem_size = phdr->p_memsz;
        segment.virt_addr = (void*) phdr->p_vaddr;
        segment.src = (void*)((uint8_t*) elf_data + phdr->p_offset);

        segments_arr[seg_count++] = segment;
    }

    segments_arr = krealloc(segments_arr, sizeof(elf_segment) * seg_count);
    
    out_info->segment_count = seg_count;
    out_info->segments = segments_arr;
    out_info->entry_point = (void*) header->e_entry;

    return true;
}