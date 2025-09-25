#include "elf/elf.h"
#include "task/task.h"
#include "heap/malloc.h"
#include "string/string.h"
#include "print/debug.h"

int exec_elf(void* elf_data) {
    elf_hdr* hdr = get_elf_hdr(elf_data);
    if (!validate_elf_hdr(hdr)) {
        debugf("[ELF] Invalid ELF header\n");
        return -1;
    }

    // Load ELF and get entry point
    uint32_t entry = load_elf(hdr);
    if (!entry) {
        debugf("[ELF] Failed to load ELF\n");
        return -2;
    }

    // Create a new task
    int tid = task_create((void (*)(void))entry);
    if (tid < 0) {
        debugf("[ELF] Failed to create task\n");
        return -3;
    }

    return tid;
}