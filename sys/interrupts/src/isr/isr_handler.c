#include "print/debug.h"
#include "libc/lib.h"
#include "isr/isr.h"
#include "panic/panic.h"
#include <stdint.h>

void isr_handler(void* stack_ptr) {
    uint32_t* stack = (uint32_t*)stack_ptr;
    
    uint32_t vector = stack[12];      // Vector number  
    uint32_t error_code = stack[13];  // Error code
    char buffer[32];

    debugf("Exception: ");
    if (vector >= 0 && vector < ISR_EXCEPTION_AMOUNT) {
        debugf(exception_messages[vector]);
    } else {
        debugf("Unknown Exception");
    }
    
    debugf(" (Vector: ");
    int2str(vector, buffer, sizeof(buffer));
    debugf(buffer);
    debugf(")");

    if (error_code != 0) {
        debugf(" | Error code: ");
        int2str(error_code, buffer, sizeof(buffer));
        debugf(buffer);
    }
    debugf("\n");

    panic(exception_messages[vector]);
}
