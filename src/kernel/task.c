/*
    MooseOS Multitasking System
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    ============================ OS THEORY: MULTITASKING ============================
    
    WHAT IS MULTITASKING?
    With multiple programs running, the CPU needs to switch between them quickly.
    This is called multitasking. The CPU gives each program a tiny "time slice"
    (like 10-100 milliseconds) before switching to the next one. It happens so fast
    that it APPEARS like all programs are running simultaneously.
    
    TWO TYPES OF MULTITASKING:
    1. COOPERATIVE MULTITASKING (what MooseOS uses):
       - Programs voluntarily give up control (like polite people taking turns)
       - A program runs until it decides to yield to others
       - If a program never yields, it can freeze the entire system!
       - Simpler to implement, but less robust
       
    2. PREEMPTIVE MULTITASKING (modern systems):
       - OS forcibly switches between programs using timer interrupts
       - Each program gets a fixed time slice whether it likes it or not
       - More complex but prevents one program from hogging the CPU
    
    TASK CONTROL BLOCKS (TCBs):
    Each running program needs information that contains:
    - Program ID and name
    - Current state (running, waiting, stopped)
    - CPU register values when paused
    - Memory information (which pages it owns)
    - Priority level
    
    CONTEXT SWITCHING:
    When switching between programs, the OS must:
    1. Save the current program's CPU registers
    2. Load the next program's CPU registers
    3. Switch to the new program's memory space
    4. Jump to where the program left off
    
    
    SCHEDULING:
    The OS needs to decide which program runs next. Common algorithms:
    - Round Robin: Take turns in order (fair but simple)
    - Priority: Important programs go first
    - Shortest Job First: Quick tasks before long ones
    MooseOS uses Round Robin scheduling.
    
    PROCESS vs THREAD:
    - PROCESS: A complete program with its own memory space
    - THREAD: A lightweight task within a process (shares memory)

    Source: https://wiki.osdev.org/Multitasking_Systems
*/
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

// Tick handler
void task_tick() {
    ticks++;
    task_schedule();
}

// Start task
void task_start() {
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].state == TASK_READY) {
            current_task = i;
            tasks[i].state = TASK_RUNNING;
            // Switch to the first task's stack and start running it
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
