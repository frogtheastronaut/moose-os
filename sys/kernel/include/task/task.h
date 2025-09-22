#ifndef TASK_H
#define TASK_H

#include "libc/lib.h"
// includes
typedef unsigned short uint16_t;
typedef short int16_t;

// defs
#define MAX_TASKS 16
#define STACK_SIZE 4096

typedef void (*task_func)();

// task states
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_FINISHED
} task_state;

typedef struct {
    uint32_t* stack_ptr;
    void (*entry)(void);
    task_state state;
    uint8_t stack[STACK_SIZE];
} task;

void task_init();
int task_create(void (*entry)(void));
void task_yield();
void task_schedule();

extern volatile uint32_t ticks;

extern void task_switch(uint32_t **old_sp, uint32_t *new_sp);

void task_start(void);

// Simple task registration system
void register_task(task_func task);
void run_tasks(void);

#endif // TASK_H

