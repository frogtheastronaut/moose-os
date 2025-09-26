#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <stdint.h>

// Launcher error codes
#define LAUNCHER_SUCCESS        0
#define LAUNCHER_FILE_NOT_FOUND -1
#define LAUNCHER_INVALID_ELF    -2
#define LAUNCHER_TASK_FAILED    -3
#define LAUNCHER_OUT_OF_MEMORY  -4

// Function prototypes
void launcher_init(void);
int launch_program(const char* filename);
void launcher_status(void);
void launcher_cleanup_program(int task_id);
void* load_file_to_memory(const char* filename, size_t* file_size);

#endif // LAUNCHER_H