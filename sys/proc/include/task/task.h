#ifndef TASK_H
#define TASK_H

#include "libc/lib.h"
#include "paging/paging.h"
// includes
typedef unsigned short uint16_t;
typedef short int16_t;

// defs
#define MAX_TASKS 16
#define STACK_SIZE 8192  // Increased for safety
#define USER_STACK_TOP 0xC0000000  // 3GB - top of user stack

typedef void (*task_func)();

// task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_FINISHED,
    TASK_BLOCKED
} task_state;

typedef struct {
    uint32_t task_id;
    uint32_t* stack_ptr;
    void (*entry)(void);
    task_state state;
    page_directory_t* page_directory;  // Each task has its own virtual memory
    uint32_t* kernel_stack;  // Kernel mode stack
    uint8_t stack[STACK_SIZE];
    void* elf_data;  // For cleanup
} task;

void task_init();
int task_create(void (*entry)(void));
int task_create_from_elf(void* elf_data);
void task_yield();
void task_schedule();
void task_cleanup(int task_id);
int task_get_current_id();
page_directory_t* task_get_page_directory(int task_id);
void task_exit();

extern volatile uint32_t ticks;

extern void task_switch(uint32_t **old_sp, uint32_t *new_sp);

// Simple task registration system
void register_task(task_func task);
void run_tasks(void);

#endif // TASK_H

