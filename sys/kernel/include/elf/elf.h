/*
    MooseOS ELF parser
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef ELF_H
#define ELF_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// ELF identification indices
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_OSABI    7
#define EI_ABIVERSION 8
#define EI_PAD      9
#define EI_NIDENT   16

// ELF magic values
#define EI_MAG0_VALUE   0x7F
#define EI_MAG1_VALUE   'E'
#define EI_MAG2_VALUE   'L'
#define EI_MAG3_VALUE   'F'

// ELF class
#define ELFCLASSNONE    0
#define ELFCLASS32      1
#define ELFCLASS64      2

// ELF data encoding
#define ELFDATANONE     0
#define ELFDATA2LSB     1
#define ELFDATA2MSB     2

// ELF version
#define EV_NONE         0
#define EV_CURRENT      1

// ELF file types
#define ET_NONE         0
#define ET_REL          1
#define ET_EXEC         2
#define ET_DYN          3
#define ET_CORE         4

// machine types
#define EM_NONE         0
#define EM_M32          1
#define EM_SPARC        2
#define EM_386          3
#define EM_68K          4
#define EM_88K          5
#define EM_860          7
#define EM_MIPS         8

// program header types
#define PT_NULL         0
#define PT_LOAD         1
#define PT_DYNAMIC      2
#define PT_INTERP       3
#define PT_NOTE         4
#define PT_SHLIB        5
#define PT_PHDR         6

// program header flags
#define PF_X            0x1
#define PF_W            0x2
#define PF_R            0x4

// ELF header
typedef struct {
    uint8_t     e_ident[EI_NIDENT];
    uint16_t    e_type;
    uint16_t    e_machine;
    uint32_t    e_version;
    uint32_t    e_entry;
    uint32_t    e_phoff;
    uint32_t    e_shoff;
    uint32_t    e_flags;
    uint16_t    e_ehsize;
    uint16_t    e_phentsize;
    uint16_t    e_phnum;
    uint16_t    e_shentsize;
    uint16_t    e_shnum;
    uint16_t    e_shstrndx;
} elf_header;

// ELF program header
typedef struct {
    uint32_t    p_type;
    uint32_t    p_offset;
    uint32_t    p_vaddr;
    uint32_t    p_paddr;
    uint32_t    p_filesz;
    uint32_t    p_memsz;
    uint32_t    p_flags;
    uint32_t    p_align;
} elf_phdr;

// ELF segment
typedef struct {
    void*       virt_addr;
    void*       src;
    size_t      file_size;
    size_t      mem_size;
    uint32_t    flags;
} elf_segment;

// ELF load info
typedef struct {
    elf_segment*  segments;
    size_t          segment_count;
    void*           entry_point;
} elf_load_info;

// parse elf files
bool elf_parse(void* elf_data, elf_load_info* out_info);

#endif // ELF_H