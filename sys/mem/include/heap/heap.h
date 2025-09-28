#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

// Memory allocation functions
void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nelem, size_t elsize);
void *krealloc(void *ptr, size_t size);
void *nofree_malloc(size_t size);

#endif