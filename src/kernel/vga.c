/*
	MooseOS VGA Palette Programming
	Copyright (c) 2025 Ethan Zhang
*/

#include "include/vga.h"

void vga_set_palette_color(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, r >> 2);  // VGA DAC expects 6-bit values (0-63)
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}

void vga_get_palette_color(uint8_t index, uint8_t* r, uint8_t* g, uint8_t* b) {
    outb(VGA_DAC_READ_INDEX, index);
    *r = inb(VGA_DAC_DATA) << 2;  // Convert back to 8-bit
    *g = inb(VGA_DAC_DATA) << 2;
    *b = inb(VGA_DAC_DATA) << 2;
}

void vga_load_palette(const uint8_t palette[256][3]) {
    for (int i = 0; i < 256; i++) {
        vga_set_palette_color(i, palette[i][0], palette[i][1], palette[i][2]);
    }
}

/**
 * Initialise a custom grayscale palette
 * @note each colour is based of a base of 'light grey'
 */
void vga_init_custom_palette(void) {
    // Base gray value from VGA_COLOR_LIGHT_GREY
    uint8_t base_gray = 170;
    
    vga_set_palette_color(VGA_COLOR_BLACK, 0, 0, 0);                           // Pure black
    vga_set_palette_color(VGA_COLOR_BLUE, base_gray - 85, base_gray - 85, base_gray - 85);     // Dark gray
    vga_set_palette_color(VGA_COLOR_GREEN, base_gray - 70, base_gray - 70, base_gray - 70);    // Medium-dark gray
    vga_set_palette_color(VGA_COLOR_CYAN, base_gray - 55, base_gray - 55, base_gray - 55);     // Medium gray
    vga_set_palette_color(VGA_COLOR_RED, base_gray - 40, base_gray - 40, base_gray - 40);      // Medium-light gray
    vga_set_palette_color(VGA_COLOR_MAGENTA, base_gray - 25, base_gray - 25, base_gray - 25);  // Light-medium gray
    vga_set_palette_color(VGA_COLOR_BROWN, base_gray - 10, base_gray - 10, base_gray - 10);    // Near base gray
    vga_set_palette_color(VGA_COLOR_LIGHT_GREY, base_gray, base_gray, base_gray);              // Keep original light grey
    vga_set_palette_color(VGA_COLOR_DARK_GREY, base_gray - 85, base_gray - 85, base_gray - 85); // Dark gray
    vga_set_palette_color(VGA_COLOR_LIGHT_BLUE, base_gray + 25, base_gray + 25, base_gray + 25);   // Lighter gray
    vga_set_palette_color(VGA_COLOR_LIGHT_GREEN, base_gray + 35, base_gray + 35, base_gray + 35);  // Light gray
    vga_set_palette_color(VGA_COLOR_LIGHT_CYAN, base_gray + 45, base_gray + 45, base_gray + 45);   // Very light gray
    vga_set_palette_color(VGA_COLOR_LIGHT_RED, base_gray + 55, base_gray + 55, base_gray + 55);    // Near white
    vga_set_palette_color(VGA_COLOR_LIGHT_MAGENTA, base_gray + 65, base_gray + 65, base_gray + 65); // Lighter near white
    vga_set_palette_color(VGA_COLOR_LIGHT_BROWN, base_gray + 75, base_gray + 75, base_gray + 75);   // Almost white
    vga_set_palette_color(VGA_COLOR_WHITE, 200, 200, 200);                     // Pure white
}