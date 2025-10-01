/*
    MooseOS Icon bitmaps
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef ICONS_H
#define ICONS_H

#include <stdint.h>
#include "vga/vga.h"

extern const uint8_t folder_icon[16][16];
extern const uint8_t file_icon[16][16];
extern const uint8_t terminal_icon[16][16];
extern const uint8_t cursor_icon[8][8];

// colour map for said icons
extern const uint8_t icon_colour_map[];

#endif // ICONS_H
