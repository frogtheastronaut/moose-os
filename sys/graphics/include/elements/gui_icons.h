/**
    MooseOS GUI Icons Header
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for icon and file drawing functions
*/
#ifndef GUI_ICONS_H
#define GUI_ICONS_H

#include "vga/vga.h"
#include "images/images.h"
#include "fontdef/fontdef.h"

// Icon drawing functions
void draw_icon(int x, int y, const uint8_t icon[][16], int width, int height, uint8_t bg_color);
void draw_file(int x, int y, const char* name, int is_dir, int is_selected);

#endif