/*
    MooseOS QEMU detection code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#include <stdint.h>
#include <stdbool.h>
#include "cpuid/cpuid.h"

/**
 * @note we can't use debugf() here because we haven't even proved we are in QEMU yet
 */
bool detect_qemu(void) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, &eax,&ebx,&ecx,&edx);
    // ECX bit 31 = hypervisor
    if (!(ecx & (1 << 31))) return false;

    cpuid(0x40000000, &eax,&ebx,&ecx,&edx);
    // hypervisor vendor string is usually "TCGTCGTCG" or "KVMKVMKVM"
    return (ebx || ecx || edx); // just check non-zero vendor string
}
