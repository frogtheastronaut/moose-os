#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <stdint.h>
#include "file/file.h"

// Function prototypes
void filesystem_init();
int fs_mount(uint8_t drive);
int filesys_sync();
int filesystem_format(uint8_t drive);
int filesystem_save_to_disk();
int fs_load_from_disk();
int filesys_disk_status();
void filesys_flush_cache();
int fs_get_disk_info(char *info_buffer, int buffer_size);
int filesys_get_memory_stats(char *stats_buffer, int buffer_size);
char* get_file_content(const char* filename);

#endif