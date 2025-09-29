/*
    MooseOS QEMU Detection Header
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef QEMU_H
#define QEMU_H

#include "io/io.h"
#include <stdbool.h>

// QEMU detection function
bool detect_qemu(void);

#endif