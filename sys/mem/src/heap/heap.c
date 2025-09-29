/*
    MooseOS Heap Allocator
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "heap/heap.h"
#include "string/string.h"
#include "assert/assert.h"

static char kernel_heap[1024 * 1024]; // 1MB heap
static size_t heap_offset = 0;

struct block_meta {
  size_t size;
  struct block_meta *next;
  int free;
};

#define META_SIZE sizeof(struct block_meta)

void *global_base = NULL;

void *sbrk(int increment) {
    if (increment == 0) {
        return &kernel_heap[heap_offset];
    }
    
    if (heap_offset + increment > sizeof(kernel_heap)) {
        return (void*)-1; // out of memory
    }
    
    void *old_break = &kernel_heap[heap_offset];
    heap_offset += increment;
    return old_break;
}

// sbrk some extra space every time we need it.
void *nofree_malloc(size_t size) {
  void *p = sbrk(0);
  void *request = sbrk(size);
  if (request == (void*) -1) { 
    return NULL; // sbrk failed
  } else {
    assert(p == request); // check that sbrk returned what we expected
    return p;
  }
}

// iterate through blocks until we find one that's large enough.
struct block_meta *find_free_block(struct block_meta **last, size_t size) {
  struct block_meta *current = global_base;
  while (current && !(current->free && current->size >= size)) {
    *last = current;
    current = current->next;
  }
  return current;
}

struct block_meta *request_space(struct block_meta* last, size_t size) {
  struct block_meta *block;
  block = sbrk(0);
  void *request = sbrk(size + META_SIZE);
  assert((void*)block == request); // check that sbrk returned what we expected
  if (request == (void*) -1) {
    return NULL; // sbrk failed.
  }
  
  if (last) { // NULL on first request.
    last->next = block;
  }
  block->size = size;
  block->next = NULL;
  block->free = 0;
  return block;
}

// if it's the first ever call, i.e., global_base == NULL, request_space and set global_base.
// otherwise, if we can find a free block, use it.
// if not, request_space.
void *kmalloc(size_t size) {
  struct block_meta *block;

  if (size <= 0) {
    return NULL;
  }

  if (!global_base) { // first call.
    block = request_space(NULL, size);
    if (!block) {
      return NULL;
    }
    global_base = block;
  } else {
    struct block_meta *last = global_base;
    block = find_free_block(&last, size);
    if (!block) { // failed to find free block.
      block = request_space(last, size);
      if (!block) {
	return NULL;
      }
    } else {      // found free block
      block->free = 0;
    }
  }
  
  return(block+1);
}

void *kcalloc(size_t nelem, size_t elsize) {
  size_t size = nelem * elsize;
  void *ptr = kmalloc(size);
  memset(ptr, 0, size);
  return ptr;
}

/**
 * @todo add validation for ptr
 */
struct block_meta *get_block_ptr(void *ptr) {
  return (struct block_meta*)ptr - 1;
}

void kfree(void *ptr) {
  if (!ptr) {
    return;
  }

  struct block_meta* block_ptr = get_block_ptr(ptr);
  assert(block_ptr->free == 0);
  block_ptr->free = 1;
}

void *krealloc(void *ptr, size_t size) {
  if (!ptr) { 
    // NULL ptr. realloc should act like malloc.
    return kmalloc(size);
  }

  struct block_meta* block_ptr = get_block_ptr(ptr);
  if (block_ptr->size >= size) {
    return ptr;
  }

  void *new_ptr;
  new_ptr = kmalloc(size);
  if (!new_ptr) {
    /**
     * @todo set errno 
     */
    return NULL;
  }
  memcpy(new_ptr, ptr, block_ptr->size);
  kfree(ptr);  
  return new_ptr;
}