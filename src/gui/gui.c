#include <stdint.h>
#include "../kernel/include/vga.h"

// Screen buffer in VGA mode 13h (320x200, 256 colors)
static uint8_t* vga_buffer = (uint8_t*)0xA0000;

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

/**
 * Sets a single pixel at the specified coordinates
 * 
 * @param x X-coordinate (0-319)
 * @param y Y-coordinate (0-199)
 * @param color VGA color value (0-255)
 */
void gui_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a horizontal line from (x1,y) to (x2,y)
 * 
 * @param x1 Starting X-coordinate
 * @param x2 Ending X-coordinate
 * @param y Y-coordinate
 * @param color Line color
 */
void gui_draw_hline(int x1, int x2, int y, uint8_t color) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    
    // Ensure x1 <= x2
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    // Clip to screen boundaries
    if (x1 < 0) x1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    
    // Draw the line
    for (int x = x1; x <= x2; x++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a vertical line from (x,y1) to (x,y2)
 * 
 * @param x X-coordinate
 * @param y1 Starting Y-coordinate
 * @param y2 Ending Y-coordinate
 * @param color Line color
 */
void gui_draw_vline(int x, int y1, int y2, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH) return;
    
    // Ensure y1 <= y2
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    // Clip to screen boundaries
    if (y1 < 0) y1 = 0;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;
    
    // Draw the line
    for (int y = y1; y <= y2; y++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a filled rectangle
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Fill color
 */
void gui_draw_rect(int x, int y, int width, int height, uint8_t color) {
    // Clip rectangle to screen boundaries
    int x_end = x + width;
    int y_end = y + height;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x_end > SCREEN_WIDTH) x_end = SCREEN_WIDTH;
    if (y_end > SCREEN_HEIGHT) y_end = SCREEN_HEIGHT;
    
    // Draw the rectangle
    for (int j = y; j < y_end; j++) {
        for (int i = x; i < x_end; i++) {
            vga_buffer[j * SCREEN_WIDTH + i] = color;
        }
    }
}

/**
 * Draws a rectangle outline
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Border color
 */
void gui_draw_rect_outline(int x, int y, int width, int height, uint8_t color) {
    // Draw top and bottom horizontal lines
    gui_draw_hline(x, x + width - 1, y, color);
    gui_draw_hline(x, x + width - 1, y + height - 1, color);
    
    // Draw left and right vertical lines
    gui_draw_vline(x, y, y + height - 1, color);
    gui_draw_vline(x + width - 1, y, y + height - 1, color);
}

/**
 * Draws a 3D-style box with highlights and shadows
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of box
 * @param height Height of box
 * @param face_color Main color of the box
 * @param highlight_color Color for top and left edges (typically lighter)
 * @param shadow_color Color for bottom and right edges (typically darker)
 */
void gui_draw_3d_box(int x, int y, int width, int height, 
                    uint8_t face_color, 
                    uint8_t highlight_color, 
                    uint8_t shadow_color) {
    // Draw the main face
    gui_draw_rect(x + 1, y + 1, width - 2, height - 2, face_color);
    
    // Draw the top and left highlight
    gui_draw_hline(x, x + width - 1, y, highlight_color);
    gui_draw_vline(x, y, y + height - 1, highlight_color);
    
    // Draw the bottom and right shadow
    gui_draw_hline(x + 1, x + width - 1, y + height - 1, shadow_color);
    gui_draw_vline(x + width - 1, y + 1, y + height - 1, shadow_color);
}

/**
 * Draws a double-bordered box (window style)
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of box
 * @param height Height of box
 * @param outer_color Color for outer border
 * @param inner_color Color for inner border
 * @param face_color Color for inner face
 */
void gui_draw_window_box(int x, int y, int width, int height,
                        uint8_t outer_color,
                        uint8_t inner_color,
                        uint8_t face_color) {
    // Draw outer border
    gui_draw_rect_outline(x, y, width, height, outer_color);
    
    // Draw inner border
    gui_draw_rect_outline(x + 1, y + 1, width - 2, height - 2, inner_color);
    
    // Draw inner face
    gui_draw_rect(x + 2, y + 2, width - 4, height - 4, face_color);
}

/**
 * Clears the entire screen with specified color
 * 
 * @param color Color to fill the screen with
 */
void gui_clear_screen(uint8_t color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        vga_buffer[i] = color;
    }
}

/**
 * Draws a title bar for a window
 * 
 * @param x Left position of window
 * @param y Top position of window
 * @param width Width of window
 * @param title_height Height of title bar
 * @param title_color Background color of title bar
 */
void gui_draw_title_bar(int x, int y, int width, int title_height, uint8_t title_color) {
    // Draw title bar background
    gui_draw_rect(x + 2, y + 2, width - 4, title_height, title_color);
    
    // Draw a line at the bottom of the title bar
    gui_draw_hline(x + 2, x + width - 3, y + title_height + 1, VGA_COLOR_BLACK);
}

// Simple I/O functions if you need to implement them
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Initialize VGA mode 13h (320x200, 256 colors) using direct VGA register programming
 */
void gui_init() {
    // Reset VGA state
    outb(0x3C2, 0x63);
    
    // Sequence controller registers
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    
    // CRTC controller registers
    // Unlock CRTC registers
    outb(0x3D4, 0x03); outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & ~0x80);
    
    // Program CRTC registers for mode 13h
    static const uint8_t crtc_data[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };
    
    for (int i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc_data[i]);
    }
    
    // Graphics controller registers for mode 13h
    static const uint8_t gc_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t gc_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};
    
    for (int i = 0; i < 9; i++) {
        outb(0x3CE, gc_reg[i]);
        outb(0x3CF, gc_data[i]);
    }
    
    // Attribute controller registers
    // Reset attribute controller flip-flop to index state
    inb(0x3DA);
    
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
    
    // Enable display
    outb(0x3C0, 0x20);
    
    // Set the buffer pointer
    vga_buffer = (uint8_t*)0xA0000;
}

/**
 * Demo function to show all box drawing capabilities
 */
// void gui_demo() {
//     gui_init();
//     // Clear screen
//     gui_clear_screen(VGA_COLOR_BLUE);
    
//     // Draw a simple outline
//     gui_draw_rect_outline(10, 10, 100, 80, VGA_COLOR_WHITE);
    
//     // Draw a filled rectangle
//     gui_draw_rect(120, 10, 100, 80, VGA_COLOR_GREEN);
    
//     // Draw a 3D box
//     gui_draw_3d_box(10, 100, 100, 80, 
//                    VGA_COLOR_LIGHT_GREY,
//                    VGA_COLOR_WHITE, 
//                    VGA_COLOR_DARK_GREY);
    
//     // Draw a window box
//     gui_draw_window_box(120, 100, 100, 80,
//                        VGA_COLOR_BLACK,
//                        VGA_COLOR_WHITE,
//                        VGA_COLOR_LIGHT_GREY);
                       
//     // Add a title bar to the window
//     gui_draw_title_bar(120, 100, 100, 12, VGA_COLOR_BLUE);
// }