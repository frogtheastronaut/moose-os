// includes
#include <stddef.h>
#include <stdint.h>

// defs
#define MAX_TASKS 8
#define STACK_SIZE 4096

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

static task tasks[MAX_TASKS];
static int current_task = -1;
static int num_tasks = 0;

void task_init() {
    for (int i = 0; i < MAX_TASKS; ++i) {
        tasks[i].state = TASK_FINISHED;
    }
    current_task = -1;
    num_tasks = 0;
}

int task_create(void (*entry)(void)) {
    if (num_tasks >= MAX_TASKS) return -1;
    int id = num_tasks++;
    tasks[id].entry = entry;
    tasks[id].state = TASK_READY;
    // Set up stack for the new task
    uint32_t* stack_top = (uint32_t*)(tasks[id].stack + STACK_SIZE - sizeof(uint32_t));
    *stack_top = (uint32_t)entry;
    tasks[id].stack_ptr = stack_top;
    return id;
}

void task_yield() {
    task_schedule();
}

void task_schedule() {
    int next = (current_task + 1) % num_tasks;
    for (int i = 0; i < num_tasks; ++i) {
        if (tasks[next].state == TASK_READY) {
            current_task = next;
            tasks[current_task].state = TASK_RUNNING;
            // Context switch would go here (not implemented in C)
            tasks[current_task].entry();
            tasks[current_task].state = TASK_FINISHED;
        }
        next = (next + 1) % num_tasks;
    }
}
