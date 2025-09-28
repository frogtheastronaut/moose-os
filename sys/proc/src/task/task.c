/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/
#include "task/task.h"

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;

// Simple task registry
static task_func registered_tasks[MAX_TASKS];
static int registered_task_count = 0;

// Global tick counter
volatile uint32_t ticks = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TASK_FINISHED;
    }
    current_task = -1;
    num_tasks = 0;
}

// Tick handler
void task_tick() {
    ticks++;
    task_schedule();
}

void task_start() {
    if (num_tasks == 0) return;
    
    // Find the first ready task and start it
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].state == TASK_READY) {
            current_task = i;
            tasks[i].state = TASK_RUNNING;
            
            // Jump to the first task
            asm volatile (
                "movl %0, %%esp\n"
                "jmp *%1\n"
                :
                : "r"(tasks[i].stack_ptr), "r"(tasks[i].entry)
                : "memory"
            );
            break;
        }
    }
}

int task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) return -1;
    int id = num_tasks++;
    tasks[id].entry = entry;
    tasks[id].state = TASK_READY;
    
    // Set up the task's stack
    uint32_t* stack_top = (uint32_t*)(tasks[id].stack + STACK_SIZE);
    
    *(--stack_top) = (uint32_t)(uintptr_t)entry;
    
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
    if (num_tasks == 0) return; // No tasks to schedule
    
    int prev_task = current_task;
    int next = (current_task + 1) % num_tasks;
    
    // Find next ready task
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[next].state == TASK_READY) {
            // Mark previous task as ready
            if (prev_task != -1 && tasks[prev_task].state == TASK_RUNNING) {
                tasks[prev_task].state = TASK_READY;
            }
            
            // Switch to next task
            current_task = next;
            tasks[current_task].state = TASK_RUNNING;
            
            // Perform task switch if we have a previous task
            if (prev_task != -1 && prev_task != current_task) {
                task_switch(&tasks[prev_task].stack_ptr, tasks[current_task].stack_ptr);
            }
            return;
        }
        next = (next + 1) % num_tasks;
    }
}

// Register task
void register_task(task_func task) {
    if (registered_task_count < MAX_TASKS) {
        registered_tasks[registered_task_count] = task;
        registered_task_count++;
    }
}

void run_tasks(void) {
    static uint32_t current_registered_task = 0;
    
    if (registered_task_count > 0) {
        registered_tasks[current_registered_task]();
        current_registered_task = (current_registered_task + 1) % registered_task_count;
    }
}
