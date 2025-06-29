/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/gui.h"

/**
 * Draws the text editor interface
 */
void gui_draw_text_editor() {
    // Clear screen
    gui_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, VGA_COLOR_LIGHT_GREY);
    
    // Draw title bar
    gui_draw_title_bar(0, 0, SCREEN_WIDTH, 15, VGA_COLOR_BLUE);
    
    // Draw "Text Editor - " label
    char label[] = "Text Editor - ";
    gui_draw_text(10, 6, label, VGA_COLOR_WHITE);
    
    // Draw filename with scrolling if it's too long
    int label_width = gui_text_width(label);
    int filename_x = 10 + label_width;
    int max_filename_width = SCREEN_WIDTH - filename_x - 10; // Leave 10px margin on right
    gui_draw_scrolling_text(filename_x, 6, editor_filename, max_filename_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);

    
    // Draw line numbers background
    gui_draw_rect(10, 40, 25, 150, VGA_COLOR_LIGHT_GREY);
    gui_draw_vline(35, 40, 190, VGA_COLOR_DARK_GREY);
    
    // Draw text content
    int y_pos = EDITOR_START_Y + 5;
    int max_lines = EDITOR_LINES_VISIBLE;
    int total_lines = count_lines(editor_content);
    
    // Adjust scroll if cursor is out of view
    if (editor_cursor_line < editor_scroll_line) {
        editor_scroll_line = editor_cursor_line;
    } else if (editor_cursor_line >= editor_scroll_line + max_lines) {
        editor_scroll_line = editor_cursor_line - max_lines + 1;
    }
    
    // Draw visible lines
    for (int line = 0; line < max_lines && (editor_scroll_line + line) < total_lines; line++) {
        int actual_line = editor_scroll_line + line;
        
        // Draw line number
        char line_num[8];
        int_to_str(actual_line + 1, line_num, sizeof(line_num));
        gui_draw_text(12, y_pos, line_num, VGA_COLOR_DARK_GREY);
        
        // Get line content
        const char* line_start = get_line_start(editor_content, actual_line);
        int line_length = get_line_length(line_start);
        
        // Create line buffer with wrapping
        char line_buffer[EDITOR_MAX_CHARS_PER_LINE + 1];
        int chars_to_draw = 0;
        
        // Copy characters up to the maximum line width
        for (int i = 0; i < EDITOR_MAX_CHARS_PER_LINE && i < line_length; i++) {
            line_buffer[chars_to_draw] = line_start[i];
            chars_to_draw++;
        }
        line_buffer[chars_to_draw] = '\0';
        
        // Draw line content
        gui_draw_text(EDITOR_START_X + 25, y_pos, line_buffer, VGA_COLOR_BLACK);
        
        // Draw cursor if on this line
        if (actual_line == editor_cursor_line) {
            // Calculate cursor x position based on actual character widths
            int cursor_x = EDITOR_START_X + 25;
            int cursor_col_to_show = editor_cursor_col;
            
            // If cursor is beyond visible area, don't show it (it's wrapped to next line)
            if (cursor_col_to_show <= EDITOR_MAX_CHARS_PER_LINE) {
                // Add up the widths of characters before the cursor position
                for (int i = 0; i < cursor_col_to_show && i < chars_to_draw; i++) {
                    cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1; // +1 for character spacing
                }
                
                if ((get_ticks() / 15) % 2 == 0) { // Blinking cursor
                    gui_draw_vline(cursor_x, y_pos, y_pos + 10, VGA_COLOR_BLACK);
                }
            }
        }
        
        y_pos += EDITOR_LINE_HEIGHT;
    }
    
    // Draw status bar
    gui_draw_rect(0, 190, SCREEN_WIDTH, 10, VGA_COLOR_DARK_GREY);
    
    // Show cursor position and scroll info
    char status[64] = "Line: ";
    char line_str[8], col_str[8];
    int_to_str(editor_cursor_line + 1, line_str, sizeof(line_str));
    int_to_str(editor_cursor_col + 1, col_str, sizeof(col_str));
    strcat(status, line_str);
    strcat(status, ", Col: ");
    strcat(status, col_str);
    
    gui_draw_text(5, 192, status, VGA_COLOR_WHITE);
    
    // Show file size
    char file_size_str[32] = "File size: ";
    char size_str[16];
    int_to_str(strlen(editor_content), size_str, sizeof(size_str));
    strcat(file_size_str, size_str);
    strcat(file_size_str, " bytes");
    gui_draw_text(150, 192, file_size_str, VGA_COLOR_WHITE);
}



/**
 * Opens a file in the text editor
 */
void gui_open_text_editor(const char* filename) {
    // Copy filename and load content
    copyStr(editor_filename, filename);
    
    // Get file content
    char* content = get_file_content(filename);
    if (content) {
        // Copy content to editor buffer
        int i = 0;
        while (content[i] && i < MAX_CONTENT - 1) {
            editor_content[i] = content[i];
            i++;
        }
        editor_content[i] = '\0';
    } else {
        // File not found or empty, start with empty content
        editor_content[0] = '\0';
    }
    
    // Reset editor state
    editor_cursor_pos = 0;
    editor_scroll_line = 0;
    cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
    editor_modified = false;
    
    // Activate editor and deactivate explorer
    editor_active = true;
    explorer_active = false;
    
    // Draw the editor
    gui_draw_text_editor();
}