/**
    MooseOS GUI Text Header
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for text drawing and manipulation functions
*/
#ifndef GUI_TEXT_H
#define GUI_TEXT_H

#include "vga/vga.h"
#include "fontdef/fontdef.h"

// Text drawing functions
void draw_char(int x, int y, char c, uint8_t color);
void draw_text(int x, int y, const char* text, uint8_t color);
int get_textwidth(const char* text);
void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t color, uint8_t bg_color);

// Line functions
int count_lines(const char* text);
const char* get_line_start(const char* text, int line_num);
int len_line(const char* line_start);

#endif