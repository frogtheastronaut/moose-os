#ifndef ELF_H
#define ELF_H

#include <stdint.h>

#define EI_NIDENT   16

// ELF Header
typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint32_t      e_entry;
    uint32_t      e_phoff;
    uint32_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} elf_hdr;

// Program Header
typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} elf_phdr;

// Section Header
typedef struct {
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;
} elf_shdr;

// Dynamic Section
typedef struct {
    int32_t  d_tag;
    union {
        uint32_t d_val;
        uint32_t d_ptr;
    } d_un;
} elf_dynamic;

// Symbol Table Entry
typedef struct {
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    unsigned char st_info;
    unsigned char st_other;
    uint16_t st_shndx;
} elf_symbol;

// Relocation Entry with Addend
typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
    int32_t  r_addend;
} elf_relocation_addend;

// Relocation Entry without Addend
typedef struct {
    uint32_t r_offset;
    uint32_t r_info;
} elf_relocation;

// ELF Ident Indexes
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define ELFCLASS32  1

// ELF Magic Numbers
#define ELFMAG0     0x7f
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

// ELF Types
#define ET_EXEC     2
#define ET_DYN      3

// Program Header Types
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2

// Program Header Flags
#define PF_X        0x1  // Execute
#define PF_W        0x2  // Write
#define PF_R        0x4  // Read

// Dynamic Section Tags
#define DT_NULL     0
#define DT_NEEDED   1
#define DT_PLTRELSZ 2
#define DT_PLTGOT   3
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_RELA     7
#define DT_RELASZ   8
#define DT_RELAENT  9
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_JMPREL   23
#define DT_INIT_ARRAY 25
#define DT_INIT_ARRAYSZ 26

// Relocation Types (x86)
#define R_386_NONE      0
#define R_386_32        1
#define R_386_PC32      2
#define R_386_GLOB_DAT  6
#define R_386_JMP_SLOT  7
#define R_386_RELATIVE  8

// Macros to extract symbol and type from r_info
#define ELF32_R_SYM(i)  ((i) >> 8)
#define ELF32_R_TYPE(i) ((i) & 0xff)

// Forward declaration for paging
typedef uint32_t page_directory_t[1024];

elf_hdr* get_elf_hdr(void* data);
int validate_elf_hdr(elf_hdr* hdr);
uint32_t load_elf(elf_hdr* hdr);
uint32_t load_elf_with_paging(elf_hdr* hdr, page_directory_t* page_dir);
int process_dynamic_section(elf_dynamic* dynamic, page_directory_t* page_dir);
int apply_relocations(uint32_t rela_vaddr, uint32_t relasz, uint32_t relaent,
                     uint32_t symtab_vaddr, uint32_t strtab_vaddr, page_directory_t* page_dir);
void free_elf_memory(elf_hdr* hdr, page_directory_t* page_dir);


#endif // ELF_H
