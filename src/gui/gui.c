#include "../kernel/include/vga.h"
void putpixel(int pos_x, int pos_y, unsigned char VGA_COLOR)
{
    unsigned char* location = (unsigned char*)0xA0000 + 320 * pos_y + pos_x;
    *location = VGA_COLOR;
}

void test_putpixel() {
    // Draw a diagonal line from (0,0) to (99,99) with color 15 (white)
    for (int i = 0; i < 100; i++) {
        putpixel(i, i, 15);
    }
}

void draw_rect(int x, int y, int width, int height, unsigned char VGA_COLOR) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            putpixel(x + j, y + i, VGA_COLOR);
        }
    }
}

void test_draw_rect() {
    // Draw a rectangle at (10, 10) with width 50, height 30, color 12 (red)
    draw_rect(0, 0, 80, 25, VGA_COLOR_RED);
}