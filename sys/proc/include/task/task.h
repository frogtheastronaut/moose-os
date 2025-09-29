/*
    MooseOS Multitasking system
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef TASK_H
#define TASK_H

#include "libc/lib.h"

// definitions
#define MAX_TASKS 16
#define STACK_SIZE 4096

typedef unsigned short uint16_t;
typedef short int16_t;

typedef void (*task_func)();

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
void task_start(void);
void register_task(task_func task);
void run_tasks(void);

extern volatile uint32_t ticks;
extern void task_switch(uint32_t **old_sp, uint32_t *new_sp);

#endif // TASK_H

