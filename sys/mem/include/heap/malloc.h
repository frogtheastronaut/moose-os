#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>

// Memory allocation functions
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nelem, size_t elsize);
void *realloc(void *ptr, size_t size);
void *nofree_malloc(size_t size);

#endif