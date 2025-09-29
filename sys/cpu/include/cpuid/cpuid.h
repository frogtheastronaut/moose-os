/*
    MooseOS CPUID tool
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef CPUID_H
#define CPUID_H

#include <stdint.h>
void cpuid(uint32_t code, uint32_t* a, uint32_t* b, uint32_t* c, uint32_t* d);

#endif // CPUID_H