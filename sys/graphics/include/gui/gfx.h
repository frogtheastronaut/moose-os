/**
    MooseOS GUI Windows Header
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for shape and window drawing functions
*/
#ifndef GUI_WINDOWS_H
#define GUI_WINDOWS_H

#include "vga/vga.h"

// Shape drawing functions
void draw_line_horizontal(int x1, int x2, int y, uint8_t color);
void draw_line_vertical(int x, int y1, int y2, uint8_t color);
void draw_rect(int x, int y, int width, int height, uint8_t color);
void draw_rectoutline(int x, int y, int width, int height, uint8_t color);
void draw_3dbox(int x, int y, int width, int height, 
                uint8_t face_color, 
                uint8_t highlight_color, 
                uint8_t shadow_color);
void draw_windowbox(int x, int y, int width, int height,
                    uint8_t outer_color,
                    uint8_t inner_color,
                    uint8_t face_color);
void draw_title(int x, int y, int width, int height, uint8_t title_color);

extern char dialog_input[MAX_DIALOG_INPUT_LEN + 1];
extern int dialog_input_pos;
extern int dialog_type;
void draw_dialog(const char* title, const char* prompt);

#endif