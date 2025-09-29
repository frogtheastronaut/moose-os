/*
    MooseOS CLI/STI Implementation
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "stdlib/clisti.h"

void cli(void) {
    __asm__ volatile ("cli");
}

void sti(void) {
    __asm__ volatile ("sti");
}
