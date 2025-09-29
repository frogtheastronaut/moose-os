/*
    MooseOS QEMU debugf code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef DEBUG_H
#define DEBUG_H
#include <stdint.h>
#include "io/io.h"
#include <stddef.h>
#include "qemu/qemu.h"

void debugf(const char* str);
#endif // DEBUG_H