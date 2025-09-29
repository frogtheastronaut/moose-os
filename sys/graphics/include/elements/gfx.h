/*
    MooseOS GUI elements code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef GUI_WINDOWS_H
#define GUI_WINDOWS_H

#include "vga/vga.h"
#include "gui/gui.h"

// shape drawing functions
void draw_line_horizontal(int x1, int x2, int y, uint8_t colour);
void draw_line_vertical(int x, int y1, int y2, uint8_t colour);
void draw_rect(int x, int y, int width, int height, uint8_t colour);
void draw_rectoutline(int x, int y, int width, int height, uint8_t colour);
void draw_3dbox(int x, int y, int width, int height, 
                uint8_t face_colour, 
                uint8_t highlight_colour, 
                uint8_t shadow_colour);
void draw_windowbox(int x, int y, int width, int height,
                    uint8_t outer_colour,
                    uint8_t inner_colour,
                    uint8_t face_colour);
void draw_title(int x, int y, int width, int height, uint8_t title_colour);

extern char dialog_input[MAX_DIALOG_INPUT_LEN + 1];
extern int dialog_input_pos;
extern int dialog_type;
void draw_dialog(const char* title, const char* prompt);

#endif // GUI_WINDOWS_H