#include "include/task.h"

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;

// global tick counter
volatile uint32_t ticks = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TASK_FINISHED;
    }
    current_task = -1;
    num_tasks = 0;
}

// it TICKS
void task_tick() {
    ticks++;
    task_schedule();
}

// start task
void task_start() {
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].state == TASK_READY) {
            current_task = i;
            tasks[i].state = TASK_RUNNING;
            // Switch to the first task's stack and start running
            // This will not return
            asm volatile (
                "movl %0, %%esp\n"
                "call *%1\n"
                :
                : "r"(tasks[i].stack_ptr), "r"(tasks[i].entry)
            );
        }
    }
}

int task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) return -1;
    int id = num_tasks++;
    tasks[id].entry = entry;
    tasks[id].state = TASK_READY;
    uint32_t* stack_top = (uint32_t*)(tasks[id].stack + STACK_SIZE);
    *(--stack_top) = (uint32_t)(uintptr_t)entry; 
    for (int i = 0; i < 4; ++i) *(--stack_top) = 0;
    tasks[id].stack_ptr = stack_top;
    return id;
}

void task_yield() {
    task_tick();
}

void task_schedule() {
    if (num_tasks == 0) return; // no tasks to schedule
    
    int prev_task = current_task;
    int next = (current_task + 1) % num_tasks;
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[next].state == TASK_READY) {
            if (prev_task != -1 && tasks[prev_task].state == TASK_RUNNING)
                tasks[prev_task].state = TASK_READY;
            current_task = next;
            tasks[current_task].state = TASK_RUNNING;
            // save old stack, switch to new stack
            if (prev_task != -1 && prev_task != current_task) {
                task_switch(&tasks[prev_task].stack_ptr, tasks[current_task].stack_ptr);
            }
            return;
        }
        next = (next + 1) % num_tasks;
    }
}
