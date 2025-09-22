/**
 * Below are code for the mouse cursor
 * @todo: refactor this code
 */

#include "mouse_gui/mouse_gui.h"
#include "mouse/mouse.h"

static cursor_pixel_t cursor_pixels[64];
static int num_cursor_pixels = 0;

void draw_mouse(int x, int y) {
    num_cursor_pixels = 0;
    
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            int screen_x = x + i;
            int screen_y = y + j;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && 
                screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                uint8_t pattern = cursor_icon[j][i];
                // Only draw and track non-transparent pixels
                if (pattern == 1 || pattern == 2) {
                    // Save original pixel
                    cursor_pixels[num_cursor_pixels].x = screen_x;
                    cursor_pixels[num_cursor_pixels].y = screen_y;
                    cursor_pixels[num_cursor_pixels].original_color = vga_buffer[screen_y * SCREEN_WIDTH + screen_x];
                    cursor_pixels[num_cursor_pixels].is_modified = true;
                    num_cursor_pixels++;
                    
                    // Draw cursor pixel
                    if (pattern == 1) {
                        vga_buffer[screen_y * SCREEN_WIDTH + screen_x] = VGA_COLOR_BLACK;
                    } else if (pattern == 2) {
                        vga_buffer[screen_y * SCREEN_WIDTH + screen_x] = VGA_COLOR_WHITE;
                    }
                }
            }
        }
    }
}

void restore_cursor_pixels(void) {
    for (int i = 0; i < num_cursor_pixels; i++) {
        if (cursor_pixels[i].is_modified) {
            vga_buffer[cursor_pixels[i].y * SCREEN_WIDTH + cursor_pixels[i].x] = cursor_pixels[i].original_color;
        }
    }
    num_cursor_pixels = 0;
}

void update_mouse(void) {

    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    
    
    extern bool editor_active;
    if (editor_active) {
        return;
    }
    
    mouse_state_t* mouse = get_mouse_state();
    if (!mouse) return;
    
    int cursor_x = (mouse->x_position * SCREEN_WIDTH) / 640;
    int cursor_y = (mouse->y_position * SCREEN_HEIGHT) / 480;
    
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x > SCREEN_WIDTH - 8) cursor_x = SCREEN_WIDTH - 8;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y > SCREEN_HEIGHT - 8) cursor_y = SCREEN_HEIGHT - 8;
    
    // only update if moved
    if (cursor_x != last_mouse_x || cursor_y != last_mouse_y) {
        if (last_mouse_x >= 0 && last_mouse_y >= 0) {
            restore_cursor_pixels();
        }
        
        // draw cursor at new pos
        draw_mouse(cursor_x, cursor_y);
        
        // update position
        last_mouse_x = cursor_x;
        last_mouse_y = cursor_y;
    }
}

void gui_clearmouse(void) {
    if (last_mouse_x >= 0 && last_mouse_y >= 0) {
        restore_cursor_pixels();
    }
    
    last_mouse_x = -1;
    last_mouse_y = -1;
}

void gui_updatemouse(void) {
    
    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    
    update_mouse();
}

void draw_cursor(void) {
    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    last_mouse_x = -1;
    last_mouse_y = -1;
    update_mouse();
}