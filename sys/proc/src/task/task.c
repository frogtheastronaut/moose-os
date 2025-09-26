/*
    MooseOS Multitasking System
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
*/
#include "task/task.h"
#include "elf/elf.h"
#include "heap/malloc.h"
#include "string/string.h"
#include "print/debug.h"

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;
static uint32_t next_task_id = 1;

// Simple task registry
static task_func simple_tasks[MAX_TASKS];
static int simple_task_count = 0;

// global tick counter
volatile uint32_t ticks = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TASK_FINISHED;
        tasks[i].page_directory = NULL;
        tasks[i].task_id = 0;
        tasks[i].elf_data = NULL;
    }
    current_task = -1;
    num_tasks = 0;
    next_task_id = 1;
}

// Tick handler
void task_tick() {
    ticks++;
    task_schedule();
}


int task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) {
        debugf("[TASK] Maximum tasks reached\n");
        return -1;
    }
    
    int id = num_tasks++;
    tasks[id].task_id = next_task_id++;
    tasks[id].entry = entry;
    tasks[id].state = TASK_READY;
    tasks[id].elf_data = NULL;
    
    // For simple kernel tasks, use the kernel page directory
    // Only ELF tasks need their own page directories
    tasks[id].page_directory = kernel_directory;
    
    // Set up kernel stack
    tasks[id].kernel_stack = (uint32_t*)malloc(STACK_SIZE);
    if (!tasks[id].kernel_stack) {
        debugf("[TASK] Failed to allocate kernel stack\n");
        num_tasks--;
        return -1;
    }
    
    // Set up the task's stack for proper task switching
    uint32_t* stack_top = (uint32_t*)(tasks[id].stack + STACK_SIZE);
    
    // Push return address (task entry point)
    *(--stack_top) = (uint32_t)(uintptr_t)entry;
    
    // Push registers in the order that task_switch expects to pop them
    *(--stack_top) = 0; // ebp
    *(--stack_top) = 0; // ebx  
    *(--stack_top) = 0; // esi
    *(--stack_top) = 0; // edi
    
    tasks[id].stack_ptr = stack_top;
    return id;
}

int task_create_from_elf(void* elf_data) {
    if (!elf_data) {
        debugf("[TASK] Invalid ELF data\n");
        return -1;
    }
    
    elf_hdr* hdr = get_elf_hdr(elf_data);
    if (!validate_elf_hdr(hdr)) {
        debugf("[TASK] Invalid ELF header\n");
        return -1;
    }

    if (num_tasks >= MAX_TASKS) {
        debugf("[TASK] Maximum tasks reached\n");
        return -1;
    }
    
    int id = num_tasks++;
    tasks[id].task_id = next_task_id++;
    tasks[id].state = TASK_READY;
    tasks[id].elf_data = elf_data;
    
    // Create a new page directory for this task
    tasks[id].page_directory = create_page_directory();
    if (!tasks[id].page_directory) {
        debugf("[TASK] Failed to create page directory\n");
        num_tasks--;
        return -1;
    }
    
    // Load ELF with the task's page directory
    uint32_t entry_point = load_elf_with_paging(hdr, tasks[id].page_directory);
    if (!entry_point) {
        debugf("[TASK] Failed to load ELF with paging\n");
        destroy_page_directory(tasks[id].page_directory);
        num_tasks--;
        return -1;
    }
    
    tasks[id].entry = (void (*)(void))entry_point;
    
    // Set up kernel stack
    tasks[id].kernel_stack = (uint32_t*)malloc(STACK_SIZE);
    if (!tasks[id].kernel_stack) {
        debugf("[TASK] Failed to allocate kernel stack\n");
        destroy_page_directory(tasks[id].page_directory);
        num_tasks--;
        return -1;
    }
    
    // Set up the task's stack for kernel mode execution
    uint32_t* stack_top = (uint32_t*)(tasks[id].stack + STACK_SIZE);
    
    // Push return address (task entry point)
    *(--stack_top) = entry_point;
    
    // Push registers in the order that task_switch expects to pop them
    *(--stack_top) = 0; // ebp
    *(--stack_top) = 0; // ebx  
    *(--stack_top) = 0; // esi
    *(--stack_top) = 0; // edi
    
    tasks[id].stack_ptr = stack_top;
    return id;
}

void task_yield() {
    task_tick();
}

void task_schedule() {
    if (num_tasks == 0) return;
    
    static int debug_counter = 0;
    int prev_task = current_task;
    int next = (current_task + 1) % num_tasks;
    
    // Find next ready task
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[next].state == TASK_READY) {
            // Mark previous task as ready (if it was running)
            if (prev_task != -1 && tasks[prev_task].state == TASK_RUNNING) {
                tasks[prev_task].state = TASK_READY;
            }
            
            // Switch to next task
            current_task = next;
            tasks[current_task].state = TASK_RUNNING;
            
            // Debug output occasionally (first 5 times and every 10000th execution)
            if (debug_counter < 5 || (debug_counter % 10000) == 0) {
                if (tasks[current_task].elf_data) {
                    debugf("[TASK] Running ELF task\n");
                } else {
                    debugf("[TASK] Running kernel task\n");
                }
            }
            debug_counter++;
            
            // Switch page directory if different task
            if (prev_task != current_task && tasks[current_task].page_directory) {
                switch_page_directory(tasks[current_task].page_directory);
            }
            
            // Perform task switch if we have a previous task
            if (prev_task != -1 && prev_task != current_task) {
                task_switch(&tasks[prev_task].stack_ptr, tasks[current_task].stack_ptr);
            }
            return;
        }
        next = (next + 1) % num_tasks;
        
        // Check for finished tasks and clean them up
        if (tasks[next].state == TASK_FINISHED) {
            task_cleanup(next);
        }
    }
}

// Simple task registration system implementation
void register_task(task_func task) {
    if (simple_task_count < MAX_TASKS) {
        simple_tasks[simple_task_count] = task;
        simple_task_count++;
    }
}

void run_tasks(void) {
    static uint32_t current_simple_task = 0;
    static uint32_t check_counter = 0;
    static uint32_t last_elf_marker = 0;
    static bool marker_initialized = false;
    
    // Only check marker if we have ELF tasks
    bool has_elf_tasks = false;
    for (int i = 0; i < num_tasks; i++) {
        if (tasks[i].elf_data && tasks[i].state != TASK_FINISHED) {
            has_elf_tasks = true;
            break;
        }
    }
    
    // Check ELF program marker occasionally if we have ELF tasks
    if (has_elf_tasks) {
        check_counter++;
        if (check_counter % 100000 == 0) {
            volatile uint32_t *elf_marker = (volatile uint32_t*)0x800000;
            
            if (!marker_initialized) {
                // First time - just record the initial value
                last_elf_marker = *elf_marker;
                marker_initialized = true;
                if (*elf_marker == 0xDEADBEEF) {
                    debugf("[TASK] ELF program marker detected\n");
                }
            } else if (*elf_marker != last_elf_marker) {
                debugf("[TASK] ELF program is active - marker updated\n");
                last_elf_marker = *elf_marker;
            }
        }
    }
    
    // Run simple tasks
    if (simple_task_count > 0) {
        simple_tasks[current_simple_task]();
        current_simple_task = (current_simple_task + 1) % simple_task_count;
    }
}

void task_cleanup(int task_id) {
    if (task_id < 0 || task_id >= MAX_TASKS) return;
    
    task* t = &tasks[task_id];
    
    // Free ELF memory if task was created from ELF
    if (t->elf_data && t->page_directory) {
        elf_hdr* hdr = get_elf_hdr(t->elf_data);
        if (hdr) {
            free_elf_memory(hdr, t->page_directory);
        }
        free(t->elf_data);
        t->elf_data = NULL;
    }
    
    // Free kernel stack
    if (t->kernel_stack) {
        free(t->kernel_stack);
        t->kernel_stack = NULL;
    }
    
    // Destroy page directory (but not if it's the shared kernel directory)
    if (t->page_directory && t->page_directory != kernel_directory) {
        destroy_page_directory(t->page_directory);
        t->page_directory = NULL;
    }
    
    // Reset task state
    t->state = TASK_FINISHED;
    t->task_id = 0;
    t->entry = NULL;
    t->stack_ptr = NULL;
}

int task_get_current_id() {
    if (current_task >= 0 && current_task < MAX_TASKS) {
        return tasks[current_task].task_id;
    }
    return -1;
}

page_directory_t* task_get_page_directory(int task_id) {
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].task_id == task_id) {
            return tasks[i].page_directory;
        }
    }
    return NULL;
}

void task_exit() {
    if (current_task >= 0 && current_task < MAX_TASKS) {
        tasks[current_task].state = TASK_FINISHED;
        task_yield(); // Switch away from this task
    }
}
