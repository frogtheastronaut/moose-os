/*
    MooseOS VGA graphics code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "vga/vga.h"

void vga_set_palette_colour(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
    outb(VGA_DAC_WRITE_INDEX, index);
    outb(VGA_DAC_DATA, r >> 2);  // VGA DAC expects 6-bit values
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}

void vga_get_palette_colour(uint8_t index, uint8_t* r, uint8_t* g, uint8_t* b) {
    outb(VGA_DAC_READ_INDEX, index);
    *r = inb(VGA_DAC_DATA) << 2;  // convert back to 8-bit
    *g = inb(VGA_DAC_DATA) << 2;
    *b = inb(VGA_DAC_DATA) << 2;
}

void vga_load_palette(const uint8_t palette[256][3]) {
    for (int i = 0; i < 256; i++) {
        vga_set_palette_colour(i, palette[i][0], palette[i][1], palette[i][2]);
    }
}

/**
 * initialise a custom grayscale palette
 * @note each colour is based of a base of 'light grey'
 */
void vga_init_custom_palette(void) {
    // base gray value
    uint8_t base_gray = 170;

    vga_set_palette_colour(VGA_COLOUR_BLACK, 0, 0, 0);                                               // black-black-black-black
    vga_set_palette_colour(VGA_COLOUR_BLUE, base_gray - 85, base_gray - 85, base_gray - 85);         // black-black-black-grey
    vga_set_palette_colour(VGA_COLOUR_GREEN, base_gray - 70, base_gray - 70, base_gray - 70);        // black-black-black
    vga_set_palette_colour(VGA_COLOUR_CYAN, base_gray - 55, base_gray - 55, base_gray - 55);         // black-black-medium
    vga_set_palette_colour(VGA_COLOUR_RED, base_gray - 40, base_gray - 40, base_gray - 40);          // black-black-medium-light-grey
    vga_set_palette_colour(VGA_COLOUR_MAGENTA, base_gray - 25, base_gray - 25, base_gray - 25);      // black-black-light-medium-grey
    vga_set_palette_colour(VGA_COLOUR_BROWN, base_gray - 10, base_gray - 10, base_gray - 10);        // black-black-near-grey
    vga_set_palette_colour(VGA_COLOUR_LIGHT_GREY, base_gray, base_gray, base_gray);                  // black-black-grey
    vga_set_palette_colour(VGA_COLOUR_DARK_GREY, base_gray - 85, base_gray - 85, base_gray - 85);    // black-dark-grey
    vga_set_palette_colour(VGA_COLOUR_LIGHT_BLUE, base_gray + 25, base_gray + 25, base_gray + 25);   // black-light-blue
    vga_set_palette_colour(VGA_COLOUR_LIGHT_GREEN, base_gray + 35, base_gray + 35, base_gray + 35);  // black-light-green
    vga_set_palette_colour(VGA_COLOUR_LIGHT_CYAN, base_gray + 45, base_gray + 45, base_gray + 45);   // black-light-cyan
    vga_set_palette_colour(VGA_COLOUR_LIGHT_RED, base_gray + 55, base_gray + 55, base_gray + 55);    // black-light-red
    vga_set_palette_colour(VGA_COLOUR_LIGHT_MAGENTA, base_gray + 65, base_gray + 65, base_gray + 65);// black-light-magenta
    vga_set_palette_colour(VGA_COLOUR_LIGHT_BROWN, base_gray + 75, base_gray + 75, base_gray + 75);  // black-light-brown
    vga_set_palette_colour(VGA_COLOUR_WHITE, 200, 200, 200);                                         // black-white
}