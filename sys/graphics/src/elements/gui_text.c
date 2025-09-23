/*
    MooseOS GUI Text - Text Drawing and Manipulation Functions
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "gui/gui.h"
#include "elements/gfx.h"

/**
 * Draw a character
 * @param x X coordinate of the character
 * @param y Y coordinate of the character
 * @param c Character to draw
 * @param color Color of the character
 */
void draw_char(int x, int y, char c, uint8_t color) {
    const uint8_t *glyph = system_font[(unsigned char)c];
    int char_width = char_widths[(unsigned char)c];
    int offset = 0;
    if (char_width < 8) {
        offset = (8 - char_width) / 2;
    }
    
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = glyph[row];
        
        for (int col = 0; col < char_width; col++) {
            
            if (row_data & (0x80 >> (col + offset))) {
                gui_set_pixel(x + col, y + row, color);
            }
        }
    }
}

/**
 * Draw text
 * @param x X coordinate of the text
 * @param y Y coordinate of the text
 * @param text Pointer to the string to draw
 * @param color Color of the text
 */
void draw_text(int x, int y, const char* text, uint8_t color) {
    int current_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            current_x = x;
            y += 9;  
            continue;
        }
        draw_char(current_x, y, c, color);
        current_x += char_widths[c] + 1;  
        if (current_x >= SCREEN_WIDTH - 8) {
            current_x = x;
            y += 9;  
        }
    }
}
// Get width of text
int get_textwidth(const char* text) {
    int width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        width += char_widths[c] + 1;  
    }
    if (width > 0) width--;
    return width;
}

// Draw text with scrolling
void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t color, uint8_t bg_color) {
    int text_width = get_textwidth(text);
    if (text_width <= max_width) {
        draw_text(x, y, text, color);
        return;
    }
    draw_rect(x, y, max_width, 10, bg_color);
    int text_len = strlen(text);
    int i = 0;
    int current_width = text_width;
    int ellipsis_width = get_textwidth("...");
    while (i < text_len && current_width > max_width - ellipsis_width) {
        current_width -= (char_widths[(unsigned char)text[i]] + 1);
        i++;
    }
    draw_text(x, y, "...", color);
    draw_text(x + ellipsis_width, y, &text[i], color);
}

int count_lines(const char* text) {
    int lines = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }
    return lines;
}

const char* get_line_start(const char* text, int line_num) {
    if (line_num == 0) return text;
    
    int current_line = 0;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            current_line++;
            if (current_line == line_num) {
                return &text[i + 1];
            }
        }
    }
    return text; 
}

// Get line length
int len_line(const char* line_start) {
    int length = 0;
    while (line_start[length] && line_start[length] != '\n') {
        length++;
    }
    return length;
}


