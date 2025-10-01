/*
    MooseOS Font bitmaps
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef FONTDEF_H
#define FONTDEF_H

#include "stdint.h"

// defined in fontdef.c
extern const uint8_t system_font[256][8];
extern const uint8_t char_widths[256];

#endif // FONTDEF_H
