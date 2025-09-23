/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#ifndef TEST_INTERRUPTS_H
#define TEST_INTERRUPTS_H

#include <stdint.h>

// Function declarations
void test_divide_by_zero(void);
void test_null_pointer_dereference(void);
void test_invalid_memory_access(void);
void test_stack_overflow(void);
void test_protection_fault(void);
void test_page_fault(void);
void test_invalid_opcode(void);

#endif
