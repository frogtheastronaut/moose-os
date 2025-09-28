/*
    MooseOS Filesystem implementation
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include <stdint.h>
#include "file/file.h"

// function prototypes
void filesystem_init();
int filesystem_mount(uint8_t drive);
int filesystem_sync();
int filesystem_format(uint8_t drive);
int filesystem_save_to_disk();
int filesystem_load_from_disk();
int filesystem_disk_status();
void filesystem_flush_cache();
int filesystem_get_disk_info(char *info_buffer, int buffer_size);
int filesystem_get_memory_stats(char *stats_buffer, int buffer_size);
char* get_file_content(const char* filename);

#endif