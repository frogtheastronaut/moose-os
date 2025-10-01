/*
    MooseOS Multitasking system
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "task/task.h"
#include "print/debug.h"

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;

static task_func registered_tasks[MAX_TASKS];
static int registered_task_count = 0;

// global tick counter
volatile uint32_t ticks = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TASK_FINISHED;
    }
    current_task = -1;
    num_tasks = 0;
}

// tick handler
void task_tick() {
    ticks++;
    task_schedule();
}

void task_start() {
    if (num_tasks == 0) {
        debugf("[TASK] No tasks to run!\n");
        return;
    }
    
    // find the first ready task and start it
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].state == TASK_READY) {
            current_task = i;
            tasks[i].state = TASK_RUNNING;
            
            // jump to the first task with assembly
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

// create a new task
int task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) {
        debugf("[TASK] Max task limit reached!\n");
        return -1;
    }
    int id = num_tasks++;
    tasks[id].entry = entry;
    tasks[id].state = TASK_READY;
    
    // set up the task's stack
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
    if (num_tasks == 0) {
        debugf("[TASK] No tasks to schedule!\n");
        return; // no tasks to schedule
    }
    
    int prev_task = current_task;
    int next = (current_task + 1) % num_tasks;
    
    // find next ready task
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[next].state == TASK_READY) {
            // mark previous task as ready
            if (prev_task != -1 && tasks[prev_task].state == TASK_RUNNING) {
                tasks[prev_task].state = TASK_READY;
            }
            
            // switch to next task
            current_task = next;
            tasks[current_task].state = TASK_RUNNING;
            
            // perform task switch if we have a previous task
            if (prev_task != -1 && prev_task != current_task) {
                task_switch(&tasks[prev_task].stack_ptr, tasks[current_task].stack_ptr);
            }
            return;
        }
        next = (next + 1) % num_tasks;
    }
}

// register task
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
