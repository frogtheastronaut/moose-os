/*
    MooseOS Interrupt Service Routine handler code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "print/debug.h"
#include "libc/lib.h"
#include "isr/isr.h"
#include "panic/panic.h"
#include <stdint.h>

void isr_handler(void* stack_ptr) {
    uint32_t* stack = (uint32_t*)stack_ptr;
    
    uint32_t vector = stack[12];      // vector number  
    uint32_t error_code = stack[13];  // error code
    char buffer[32];

    // tell the user that they messed up
    // when really the only thing thats messed up is the code
    debugf("Exception: ");
    if (vector >= 0 && vector < ISR_EXCEPTION_AMOUNT) {
        debugf(exception_messages[vector]);
    } else {
        debugf("Unknown Exception");
    }
    
    debugf(" (Vector: ");
    int_to_str(vector, buffer, sizeof(buffer));
    debugf(buffer);
    debugf(")");

    if (error_code != 0) {
        debugf(" | Error code: ");
        int_to_str(error_code, buffer, sizeof(buffer));
        debugf(buffer);
    }
    debugf("\n");

    panic(exception_messages[vector]);
}
