/**
 * MooseOS Interrupt Service Routine test code
 * Copyright (c) 2025 Ethan Zhang
 * Licensed under the MIT license. See license file for details
 *
 * @warning These functions intentionally cause exceptions. For testing purposes only
*/

#include "isr/test_interrupts.h"
#include "panic/panic.h"
#include "print/debug.h"

void test_divide_by_zero(void) {
    debugf("[TEST] Triggering divide by zero exception\n");
    volatile int a = 10;
    volatile int b = 0;
    volatile int result = a / b;  
    (void)result; 
}

void test_null_pointer_dereference(void) {
    debugf("[TEST] Triggering null pointer dereference\n");
    volatile int* null_ptr = (int*)0x0;
    volatile int value = *null_ptr; 
    (void)value; 
}

void test_invalid_memory_access(void) {
    debugf("[TEST] Triggering invalid memory access\n");
    volatile int* invalid_ptr = (int*)0xDEADBEEF;
    volatile int value = *invalid_ptr;
    (void)value;
}

void test_stack_overflow_recursive(int depth) {
    volatile char buffer[1024];
    buffer[0] = depth;  
    debugf("[TEST] Testing stack overflow recursive");
    test_stack_overflow_recursive(depth + 1);  // Infinite recursion
}

void test_stack_overflow(void) {
    debugf("[TEST] Triggering stack overflow\n");
    test_stack_overflow_recursive(0);
}

void test_protection_fault(void) {
    debugf("[TEST] Triggering protection fault\n");
    volatile int* kernel_ptr = (int*)0x100000; 
    *kernel_ptr = 0x12345678;
}


void test_page_fault(void) {
    debugf("[TEST] Triggering page fault\n");
    volatile int* unmapped_ptr = (int*)0x80000000;  
    volatile int value = *unmapped_ptr;
    (void)value;
}

void test_invalid_opcode(void) {
    debugf("[TEST] Triggering invalid opcode\n");
    asm volatile(".byte 0x0F, 0x0B");
}
