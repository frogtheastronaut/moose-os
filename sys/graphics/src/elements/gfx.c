/*
    MooseOS GUI Windows - Shape and Window Drawing Functions
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "gui/gui.h"
#include "gui/gui_text.h"

/**
 * Draw a horizontal line
 * 
 * @param x1 Starting x coordinate
 * @param x2 Ending x coordinate
 * @param y Y coordinate
 * @param color Colour of the line
 */
void draw_line_horizontal(int x1, int x2, int y, uint8_t color) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    if (x1 < 0) x1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    
    for (int x = x1; x <= x2; x++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draw a vertical line
 * 
 * @param x X coordinate
 * @param y1 Starting Y coordinate
 * @param y2 Ending Y coordinate
 * @param color Colour of the line
 */
void draw_line_vertical(int x, int y1, int y2, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH) return;
    
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    if (y1 < 0) y1 = 0;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;
    
    for (int y = y1; y <= y2; y++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

// Draw a rectangle
// The params are self-explanatory
void draw_rect(int x, int y, int width, int height, uint8_t color) {
    int x_end = x + width;
    int y_end = y + height;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x_end > SCREEN_WIDTH) x_end = SCREEN_WIDTH;
    if (y_end > SCREEN_HEIGHT) y_end = SCREEN_HEIGHT;
    
    for (int j = y; j < y_end; j++) {
        for (int i = x; i < x_end; i++) {
            vga_buffer[j * SCREEN_WIDTH + i] = color;
        }
    }
}

// Draw a hollow rectangle
void draw_rectoutline(int x, int y, int width, int height, uint8_t color) {
    draw_line_horizontal(x, x + width - 1, y, color);
    draw_line_horizontal(x, x + width - 1, y + height - 1, color);
    
    draw_line_vertical(x, y, y + height - 1, color);
    draw_line_vertical(x + width - 1, y, y + height - 1, color);
}

/**
 * Draw a 3d-style box
 * 
 * @param x X coordinate of top-left corner
 * @param y Y coordinate of top-left corner
 * @param width Width of the box
 * @param height Height of the box
 * @param face_color Color of the box face
 * @param highlight_color Color of the highlight
 * @param shadow_color Color of the shadow
 */
void draw_3dbox(int x, int y, int width, int height, 
                    uint8_t face_color, 
                    uint8_t highlight_color, 
                    uint8_t shadow_color) {
    draw_rect(x + 1, y + 1, width - 2, height - 2, face_color);
    
    draw_line_horizontal(x, x + width - 1, y, highlight_color);
    draw_line_vertical(x, y, y + height - 1, highlight_color);
    
    draw_line_horizontal(x + 1, x + width - 1, y + height - 1, shadow_color);
    draw_line_vertical(x + width - 1, y + 1, y + height - 1, shadow_color);
}

// Draw a window box
void draw_windowbox(int x, int y, int width, int height,
                        uint8_t outer_color,
                        uint8_t inner_color,
                        uint8_t face_color) {
    
    draw_rectoutline(x, y, width, height, outer_color);
    
    
    draw_rectoutline(x + 1, y + 1, width - 2, height - 2, inner_color);
    
    
    draw_rect(x + 2, y + 2, width - 4, height - 4, face_color);
}

// Draw a title bar
void draw_title(int x, int y, int width, int height, uint8_t title_color) {
    draw_rect(x + 2, y + 2, width - 4, height - 4, title_color);
    draw_line_horizontal(x + 2, x + width - 3, y + height - 3, VGA_COLOR_BLACK);
}

// Dialog-related variables
char dialog_input[MAX_DIALOG_INPUT_LEN + 1] = "";
int dialog_input_pos = 0;
int dialog_type = 0;

// Draw a dialog
void draw_dialog(const char* title, const char* prompt) {
    int width = 200;
    int height = 80;
    
    int x = (SCREEN_WIDTH - width) / 2;
    int y = (SCREEN_HEIGHT - height) / 2;
    
    draw_rect(x + 4, y + 4, width, height, VGA_COLOR_DARK_GREY); 
    draw_windowbox(x, y, width, height,
                      VGA_COLOR_BLACK,
                      VGA_COLOR_WHITE,
                      VGA_COLOR_LIGHT_GREY);
    
    draw_title(x, y, width, 15, VGA_COLOR_BLUE);
    draw_text(x + 10, y + 4, title, VGA_COLOR_WHITE);
    draw_text(x + 10, y + 25, prompt, VGA_COLOR_BLACK);
    int input_box_width = width - 20;
    draw_rect(x + 10, y + 40, input_box_width, 14, VGA_COLOR_WHITE);
    draw_rectoutline(x + 10, y + 40, input_box_width, 14, VGA_COLOR_BLACK);
    
    int cursor_char_width = 0;
    for (int i = 0; i < dialog_input_pos && dialog_input[i] != '\0'; i++) {
        cursor_char_width += char_widths[(unsigned char)dialog_input[i]] + 1;
    }
    
    int text_width = get_textwidth(dialog_input);
    int max_visible_width = input_box_width - 4;  
    
    if (text_width <= max_visible_width) {
        draw_rect(x + 11, y + 41, input_box_width - 2, 12, VGA_COLOR_WHITE);
        draw_text(x + 12, y + 42, dialog_input, VGA_COLOR_BLACK);
        int cursor_x = x + 12 + cursor_char_width;
        draw_line_vertical(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
    } else {
        static int scroll_offset = 0;
        int padding = 10;
        int min_cursor_x = padding;
        int max_cursor_x = max_visible_width - padding;
        if (cursor_char_width < scroll_offset + min_cursor_x) {
            scroll_offset = cursor_char_width - min_cursor_x;
            if (scroll_offset < 0) scroll_offset = 0;
        } else if (cursor_char_width > scroll_offset + max_cursor_x) {
            
            scroll_offset = cursor_char_width - max_cursor_x;
        }
        draw_rect(x + 11, y + 41, input_box_width - 2, 12, VGA_COLOR_WHITE);
        int draw_x = x + 12;
        int current_width = 0;
        for (int i = 0; dialog_input[i] != '\0'; i++) {
            int char_width = char_widths[(unsigned char)dialog_input[i]] + 1;
            if (current_width + char_width > scroll_offset) {
                
                int char_x = draw_x + current_width - scroll_offset;
                
                
                if (char_x < x + 12 + max_visible_width && char_x + char_width > x + 12) {
                    draw_char(char_x, y + 42, dialog_input[i], VGA_COLOR_BLACK);
                }
            }
            current_width += char_width;
            if (current_width - scroll_offset > max_visible_width) {
                break;
            }
        }
        
        int cursor_x = x + 12 + (cursor_char_width - scroll_offset);
        draw_line_vertical(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
    }
    
    draw_text(x + 10, y + 62, "ENTER: OK", VGA_COLOR_DARK_GREY);
    draw_text(x + width - 70, y + 62, "ESC: Cancel", VGA_COLOR_DARK_GREY);
}
