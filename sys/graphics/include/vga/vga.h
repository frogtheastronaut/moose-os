/*
    MooseOS VGA code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef I386_VGA_H
#define I386_VGA_H

#include <stdint.h>
#include "io/io.h"

// colours
enum vga_colour {
	VGA_COLOUR_BLACK = 0,
	VGA_COLOUR_BLUE = 1,
	VGA_COLOUR_GREEN = 2,
	VGA_COLOUR_CYAN = 3,
	VGA_COLOUR_RED = 4,
	VGA_COLOUR_MAGENTA = 5,
	VGA_COLOUR_BROWN = 6,
	VGA_COLOUR_LIGHT_GREY = 7,
	VGA_COLOUR_DARK_GREY = 8,
	VGA_COLOUR_LIGHT_BLUE = 9,
	VGA_COLOUR_LIGHT_GREEN = 10,
	VGA_COLOUR_LIGHT_CYAN = 11,
	VGA_COLOUR_LIGHT_RED = 12,
	VGA_COLOUR_LIGHT_MAGENTA = 13,
	VGA_COLOUR_LIGHT_BROWN = 14,
	VGA_COLOUR_WHITE = 15,
};

static inline uint8_t vga_entry_colour(enum vga_colour fg, enum vga_colour bg) {
	// lower 4 bits are foreground, upper 4 bits are background
	return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char uc, uint8_t colour) {
	// lower 8 bits are character, upper 8 bits are colour
	return (uint16_t) uc | (uint16_t) colour << 8;
}

// VGA DAC registers
#define VGA_DAC_READ_INDEX  0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA        0x3C9

// VGA palette programming functions
void vga_set_palette_colour(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void vga_get_palette_colour(uint8_t index, uint8_t* r, uint8_t* g, uint8_t* b);
void vga_load_palette(const uint8_t palette[256][3]);
void vga_init_custom_palette(void);

#endif