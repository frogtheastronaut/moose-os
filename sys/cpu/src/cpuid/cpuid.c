/*
    MooseOS CPUID tool
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#include "cpuid/cpuid.h"

void cpuid(uint32_t code, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d) {
    asm volatile("cpuid" : "=a"(*a), "=b"(*b), "=c"(*c), "=d"(*d) : "a"(code));
}