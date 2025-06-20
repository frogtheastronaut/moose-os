/*
	SimpleM
	Copyright 2025 Ethan Zhang
*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "include/vga.h"
#include "../lib/lib.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000 

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;
int no_delete = 0; 

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
		if (++terminal_row == VGA_HEIGHT)
			terminal_row = 0;
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
