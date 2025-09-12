/*
	MooseOS
	Copyright (c) 2025 Ethan Zhang
*/
#ifndef I386_VGA_H
#define I386_VGA_H

#include <stdint.h>
#include "../../lib/include/io.h"

// colors
enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
	return (uint16_t) uc | (uint16_t) color << 8;
}

// VGA DAC registers
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9

// VGA palette programming functions
void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vga_get_palette_color(uint8_t index, uint8_t* r, uint8_t* g, uint8_t* b);
void vga_load_palette(const uint8_t palette[256][3]);
void vga_init_custom_palette(void);

#endif