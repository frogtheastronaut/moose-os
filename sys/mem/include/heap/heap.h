/*
    MooseOS heap memory allocator
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

// memory allocation functions
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nelem, size_t elsize);
void *krealloc(void *ptr, size_t size);
void *nofree_malloc(size_t size);

#endif // MALLOC_H