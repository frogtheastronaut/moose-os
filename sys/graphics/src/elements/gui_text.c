/*
    MooseOS GUI text code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "gui/gui.h"
#include "elements/gfx.h"

/**
 * draw a character
 * @param x X coordinate of the character
 * @param y Y coordinate of the character
 * @param c character to draw
 * @param colour colour of the character
 */
void draw_char(int x, int y, char c, uint8_t colour) {
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
                gui_set_pixel(x + col, y + row, colour);
            }
        }
    }
}

/**
 * draw text
 * @param x X coordinate of the text
 * @param y Y coordinate of the text
 * @param text pointer to the string to draw
 * @param colour colour of the text
 */
void draw_text(int x, int y, const char* text, uint8_t colour) {
    int current_x = x;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\n') {
            current_x = x;
            y += 9;  
            continue;
        }
        draw_char(current_x, y, c, colour);
        current_x += char_widths[c] + 1;  
        if (current_x >= SCREEN_WIDTH - 8) {
            current_x = x;
            y += 9;  
        }
    }
}
// get width of text
int draw_text_width(const char* text) {
    int width = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        width += char_widths[c] + 1;  
    }
    if (width > 0) width--;
    return width;
}

// draw text with scrolling
void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t colour, uint8_t bg_colour) {
    int text_width = draw_text_width(text);
    if (text_width <= max_width) {
        draw_text(x, y, text, colour);
        return;
    }
    draw_rect(x, y, max_width, 10, bg_colour);
    int text_len = strlen(text);
    int i = 0;
    int current_width = text_width;
    int ellipsis_width = draw_text_width("...");
    while (i < text_len && current_width > max_width - ellipsis_width) {
        current_width -= (char_widths[(unsigned char)text[i]] + 1);
        i++;
    }
    draw_text(x, y, "...", colour);
    draw_text(x + ellipsis_width, y, &text[i], colour);
}
// count number of lines in text
int count_lines(const char* text) {
    int lines = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }
    return lines;
}
// get line start
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

// get line length
int len_line(const char* line_start) {
    int length = 0;
    while (line_start[length] && line_start[length] != '\n') {
        length++;
    }
    return length;
}


