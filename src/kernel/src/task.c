/*
    MooseOS Multitasking System
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    ============================ OS THEORY ============================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.
    
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
#include "../include/task.h"

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;

// Simple task registry
static task_func simple_tasks[MAX_TASKS];
static int simple_task_count = 0;

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

// Start multitasking system
void task_start() {
    if (num_tasks == 0) return;
    
    // Find the first ready task and start it
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[i].state == TASK_READY) {
            current_task = i;
            tasks[i].state = TASK_RUNNING;
            
            // Jump to the first task - this will start the multitasking
            // The task will call task_yield() which will trigger task_schedule()
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

void task_yield() {
    task_tick();
}

void task_schedule() {
    if (num_tasks == 0) return; // no tasks to schedule
    
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
            
            // Perform task switch if we have a previous task
            if (prev_task != -1 && prev_task != current_task) {
                task_switch(&tasks[prev_task].stack_ptr, tasks[current_task].stack_ptr);
            }
            return;
        }
        next = (next + 1) % num_tasks;
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
    
    if (simple_task_count > 0) {
        simple_tasks[current_simple_task]();
        current_simple_task = (current_simple_task + 1) % simple_task_count;
    }
}
