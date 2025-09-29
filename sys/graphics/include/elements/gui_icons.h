/*
    MooseOS GUI Icons
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef GUI_ICONS_H
#define GUI_ICONS_H

#include "vga/vga.h"
#include "icons/icons.h"
#include "fontdef/fontdef.h"

// icon drawing functions
void draw_icon(int x, int y, const uint8_t icon[][16], int width, int height, uint8_t bg_colour);
void draw_file(int x, int y, const char* name, int is_dir, int is_selected);

#endif // GUI_ICONS_H