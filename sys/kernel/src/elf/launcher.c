/*
    MooseOS ELF Launcher
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    This file implements an ELF program launcher that can load and execute
    ELF files from the filesystem, creating isolated tasks with proper
    virtual memory management.
*/

#include "elf/elf.h"
#include "task/task.h"
#include "filesystem/filesystem.h"
#include "file/file.h"
#include "heap/malloc.h"
#include "string/string.h"
#include "print/debug.h"

// Launcher error codes
#define LAUNCHER_SUCCESS        0
#define LAUNCHER_FILE_NOT_FOUND -1
#define LAUNCHER_INVALID_ELF    -2
#define LAUNCHER_TASK_FAILED    -3
#define LAUNCHER_OUT_OF_MEMORY  -4

// Constants
#define MAX_ELF_SIZE  0x10000000  // 256MB max ELF size

typedef struct {
    char* filename;
    void* data;
    size_t size;
    int task_id;
} loaded_program;

// Global array to track loaded programs
static loaded_program loaded_programs[MAX_TASKS];
static int num_loaded = 0;

/**
 * Get file content and size from filesystem
 */
char* get_file_content_with_size(const char* filename, size_t* actual_size) {
    extern File* cwd; // Reference to current working directory
    
    if (!cwd->folder.children || !actual_size) return NULL;
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        if (!cwd->folder.children[i]) continue;
        
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, filename)) {
            *actual_size = child->file.content_size;
            return child->file.content;
        }
    }
    return NULL;
}

/**
 * Load ELF file from filesystem into memory
 */
void* load_file_to_memory(const char* filename, size_t* file_size) {
    if (!filename || !file_size) {
        debugf("[LAUNCHER] Invalid parameters for file loading\n");
        return NULL;
    }
    
    // Get file content and actual size from filesystem
    size_t actual_file_size = 0;
    char* file_content = get_file_content_with_size(filename, &actual_file_size);
    if (!file_content || actual_file_size == 0) {
        debugf("[LAUNCHER] Failed to read file from filesystem\n");
        return NULL;
    }
    
    // Validate ELF header before proceeding
    if (actual_file_size < sizeof(elf_hdr)) {
        debugf("[LAUNCHER] File too small to be valid ELF\n");
        return NULL;
    }
    
    elf_hdr* hdr = (elf_hdr*)file_content;
    if (!validate_elf_hdr(hdr)) {
        debugf("[LAUNCHER] File is not a valid ELF\n");
        return NULL;
    }
    
    // Use actual file size, but add small buffer for alignment
    size_t alloc_size = actual_file_size + 1024; // Small buffer for safety
    if (alloc_size > MAX_ELF_SIZE) {
        debugf("[LAUNCHER] File too large\n");
        return NULL;
    }
    
    // Allocate memory and copy file content
    void* elf_data = malloc(alloc_size);
    if (!elf_data) {
        debugf("[LAUNCHER] Failed to allocate memory for ELF\n");
        return NULL;
    }
    
    // Copy the actual file content
    memcpy(elf_data, file_content, actual_file_size);
    
    // Zero out the buffer area
    memset((uint8_t*)elf_data + actual_file_size, 0, alloc_size - actual_file_size);
    
    *file_size = alloc_size;
    return elf_data;
}

/**
 * Launch an ELF program from filesystem
 */
int launch_program(const char* filename) {
    if (!filename) {
        debugf("[LAUNCHER] NULL filename provided\n");
        return LAUNCHER_INVALID_ELF;
    }
    
    debugf("[LAUNCHER] Loading program: ");
    debugf(filename);
    debugf("\n");
    
    // Load file into memory
    size_t file_size;
    void* elf_data = load_file_to_memory(filename, &file_size);
    if (!elf_data) {
        debugf("[LAUNCHER] Failed to load file\n");
        return LAUNCHER_FILE_NOT_FOUND;
    }
    
    // Validate ELF header
    elf_hdr* hdr = get_elf_hdr(elf_data);
    if (!validate_elf_hdr(hdr)) {
        debugf("[LAUNCHER] Invalid ELF file\n");
        free(elf_data);
        return LAUNCHER_INVALID_ELF;
    }
    
    // Create task from ELF
    int task_id = task_create_from_elf(elf_data);
    if (task_id < 0) {
        debugf("[LAUNCHER] Failed to create task from ELF\n");
        free(elf_data);
        return LAUNCHER_TASK_FAILED;
    }
    
    // Track the loaded program
    if (num_loaded < MAX_TASKS) {
        loaded_programs[num_loaded].filename = malloc(strlen(filename) + 1);
        if (loaded_programs[num_loaded].filename) {
            strcpy(loaded_programs[num_loaded].filename, filename);
            loaded_programs[num_loaded].data = elf_data;
            loaded_programs[num_loaded].size = file_size;
            loaded_programs[num_loaded].task_id = task_id;
            num_loaded++;
        }
    }
    
    debugf("[LAUNCHER] Program launched successfully with task ID: ");
    char id_str[16];
    itoa(task_id, id_str, 10);
    debugf(id_str);
    debugf("\n");
    
    return task_id;
}



/**
 * Initialize the launcher system
 */
void launcher_init() {
    debugf("[LAUNCHER] Initializing ELF launcher\n");
    
    // Initialize loaded programs array
    for (int i = 0; i < MAX_TASKS; i++) {
        loaded_programs[i].filename = NULL;
        loaded_programs[i].data = NULL;
        loaded_programs[i].size = 0;
        loaded_programs[i].task_id = -1;
    }
    num_loaded = 0;
    
    debugf("[LAUNCHER] Launcher initialized\n");
}

/**
 * Clean up a loaded program by task ID
 */
void launcher_cleanup_program(int task_id) {
    for (int i = 0; i < num_loaded; i++) {
        if (loaded_programs[i].task_id == task_id) {
            if (loaded_programs[i].filename) {
                free(loaded_programs[i].filename);
                loaded_programs[i].filename = NULL;
            }
            
            // Note: ELF data cleanup is handled by task_cleanup()
            loaded_programs[i].data = NULL;
            loaded_programs[i].size = 0;
            loaded_programs[i].task_id = -1;
            
            // Shift remaining programs
            for (int j = i; j < num_loaded - 1; j++) {
                loaded_programs[j] = loaded_programs[j + 1];
            }
            num_loaded--;
            break;
        }
    }
}



/**
 * Get status of loaded programs
 */
void launcher_status() {
    debugf("[LAUNCHER] === Loaded Programs Status ===\n");
    debugf("[LAUNCHER] Number of loaded programs: ");
    char count_str[16];
    itoa(num_loaded, count_str, 10);
    debugf(count_str);
    debugf("\n");
    
    for (int i = 0; i < num_loaded; i++) {
        if (loaded_programs[i].filename) {
            debugf("[LAUNCHER] Program: ");
            debugf(loaded_programs[i].filename);
            debugf(" Task ID: ");
            char task_id_str[16];
            itoa(loaded_programs[i].task_id, task_id_str, 10);
            debugf(task_id_str);
            debugf("\n");
        }
    }
    
    debugf("[LAUNCHER] ================================\n");
}
