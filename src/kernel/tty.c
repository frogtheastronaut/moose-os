
/*
    Moose Operating System & SimpleM
    Copyright (c) 2025 Ethan Zhang.

	This file is mostly deprecated, as I am using GUI right now.
	Nevertheless, I will keep it for reference.
*/



#include "include/vga.h"
#include "../lib/lib.h"

#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif

typedef unsigned short uint16_t;
typedef short int16_t;

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 



size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;
int no_delete = 0; 

void terminal_scroll(int lines) {
    if (lines <= 0) return;
    // Move each line up
    for (size_t y = 0; y < VGA_HEIGHT - lines; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = terminal_buffer[(y + lines) * VGA_WIDTH + x];
        }
    }
    // Clear the last 'lines' lines
    for (size_t y = VGA_HEIGHT - lines; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_buffer[y * VGA_WIDTH + x] = vga_entry(' ', terminal_color);
        }
    }
    if (terminal_row >= lines) {
        terminal_row -= lines;
    } else {
        terminal_row = 0;
    }
}

void terminal_initialize(void) 
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
	
	for (size_t y = 0; y < VGA_HEIGHT; y++) {
		for (size_t x = 0; x < VGA_WIDTH; x++) {
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = vga_entry(' ', terminal_color);
		}
	}
}

void terminal_setcolor(uint8_t color) 
{
	terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) 
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT) {
            terminal_scroll(1);
            terminal_row = VGA_HEIGHT - 1;
        }
    }
}

void terminal_write(const char* data, size_t size) 
{
	for (size_t i = 0; i < size; i++)
		terminal_putchar(data[i]);
}

void terminal_writestring(const char* data, bool newline) 
{
	terminal_write(data, strlen(data));
	if (newline == true) {
		for (int i = 0; i < VGA_WIDTH - strlen(data); i++) {
			terminal_write(" ", strlen(" "));
		}
	}
	
}
void terminal_newline()
{
	for (int i = 0; i < terminal_column; i++) {
		terminal_write(" ", strlen(" "));
	}
}
void terminal_backspace()
{
	if (terminal_column > no_delete) {
		terminal_putentryat((char)' ', terminal_color, terminal_column - 1, terminal_row );
		terminal_column--;
	}
}

