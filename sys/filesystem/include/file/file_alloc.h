/*
    MooseOS File Allocation code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/
#ifndef FILE_ALLOC_H
#define FILE_ALLOC_H
#include "file.h"

// function declarations
File* file_alloc();
void file_free(File* file);

#endif // FILE_ALLOC_H