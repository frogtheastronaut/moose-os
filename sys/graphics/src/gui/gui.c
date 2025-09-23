/*
    MooseOS Graphical User Interface (GUI)
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/


/**
    @todo: Refactor the code so its better documented. 
           Move some functions to other files as they don't necessarily belong here
 */

#include "gui/gui.h"
#include "gui/gfx.h"
#include "gui/gui_text.h"
#include "gui/gui_icons.h"

// Variables
uint8_t* vga_buffer = (uint8_t*)0xA0000;

// Set a pixel on the screen
void gui_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}



// Clear the screen
void gui_clear(uint8_t color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        vga_buffer[i] = color;
    }
}



// Initialise GUI
void gui_init() {
    // Setup the MOR (Miscellaneous Output Register)
    outb(0x3C2, 0x63);

    // Setup the Sequencer Registers
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);

    // Unlock the CRT Controller registers for modification
    outb(0x3D4, 0x03); outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & ~0x80);
    

    static const uint8_t crtc_data[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };
    // Set the CRTC registers
    for (int i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc_data[i]);
    }

    // Set the Graphics Controller registers
    static const uint8_t gc_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t gc_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};
    
    for (int i = 0; i < 9; i++) {
        outb(0x3CE, gc_reg[i]);
        outb(0x3CF, gc_data[i]);
    }

    // Prepare attribute controller
    inb(0x3DA);
    
    // Set Attribute Controller registers
    static const uint8_t ac_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    static const uint8_t ac_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41, 0x00, 0x0F, 0x00, 0x00
    };
    
    for (int i = 0; i < 21; i++) {
        outb(0x3C0, ac_reg[i]);
        outb(0x3C0, ac_data[i]);
    }
    
    // Enable video output
    outb(0x3C0, 0x20);
    
    // Pointer towards VGA graphics memory
    vga_buffer = (uint8_t*)0xA0000;
}












