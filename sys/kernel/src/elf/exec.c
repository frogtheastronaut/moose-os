#include "elf/elf.h"
#include "task/task.h"
#include "heap/malloc.h"
#include "string/string.h"
#include "print/debug.h"

int exec_elf(void* elf_data) {
    if (!elf_data) {
        debugf("[ELF] NULL ELF data\n");
        return -1;
    }

    // Create a new task from ELF data (includes validation and loading)
    int tid = task_create_from_elf(elf_data);
    if (tid < 0) {
        debugf("[ELF] Failed to create task from ELF\n");
        return tid;
    }

    return tid;
}