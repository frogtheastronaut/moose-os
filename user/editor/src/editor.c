/**
 * Moose Operating System
 * Copyright (c) 2025 Ethan Zhang and Contributors.
 * @todo: Simplify the logic
 */

#include "editor.h"

// Editor variables
char editor_content[MAX_CONTENT] = "";
char editor_filename[MAX_NAME_LEN] = "";
int editor_cursor_pos = 0;
int editor_scroll_line = 0;
int editor_cursor_line = 0;
int editor_cursor_col = 0;
bool editor_modified = false;

/**
 * Draw editor
 */
void editor_draw() {
    // Clear screen
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, VGA_COLOUR_LIGHT_GREY);
    
    // Title bar
    draw_title(0, 0, SCREEN_WIDTH, 15, VGA_COLOUR_BLUE);
    
    // Label on title bar
    char label[] = "Text Editor - ";
    draw_text(10, 6, label, VGA_COLOUR_WHITE);
    
    // File name, scroll if too big
    int label_width = get_textwidth(label);
    int filename_x = 10 + label_width;
    int max_filename_width = SCREEN_WIDTH - filename_x - 10;
    draw_text_scroll(filename_x, 6, editor_filename, max_filename_width, VGA_COLOUR_WHITE, VGA_COLOUR_BLUE);


    // Draw numbers area (line numbers)
    draw_rect(0, 20, EDITOR_LINE_NUM_WIDTH, SCREEN_HEIGHT - 20, VGA_COLOUR_LIGHT_GREY);
    draw_line_vertical(EDITOR_LINE_NUM_WIDTH, 20, SCREEN_HEIGHT, VGA_COLOUR_DARK_GREY);

    // Draw content with scrolling
    int y_pos = EDITOR_START_Y + 5;
    int max_lines = EDITOR_LINES_VISIBLE;
    int total_lines = count_lines(editor_content);
    int editor_width = EDITOR_WIDTH - EDITOR_LINE_NUM_WIDTH - 10; // Full width minus line numbers and padding 
    

    if (editor_scroll_line >= total_lines) {
        editor_scroll_line = total_lines - 1;
        if (editor_scroll_line < 0) editor_scroll_line = 0;
    }
    // Ensure visibility
    editor_cursor_visible();

    // Draw visible lines with wrapping
    int visible_line_count = 0;
    int screen_lines_used = 0;  
    
    for (int line = editor_scroll_line; line < total_lines && screen_lines_used < max_lines; line++) {
        // Get line content
        const char* line_start = get_line_start(editor_content, line);
        int line_length = len_line(line_start);
        
        if (line_length == 0) {
            // Draw line number
            char line_num[8];
            int2str(line + 1, line_num, sizeof(line_num));
            draw_text(10, y_pos, line_num, VGA_COLOUR_DARK_GREY);

            // Draw cursor if it's on this empty line
            if (line == editor_cursor_line && editor_cursor_col == 0) {
                int cursor_x = EDITOR_LINE_NUM_WIDTH + 5; // Start after line numbers
                draw_line_vertical(cursor_x, y_pos, y_pos + 8, VGA_COLOUR_BLUE);
            }
            
            y_pos += EDITOR_LINE_HEIGHT;
            screen_lines_used++;
            continue;
        }
        
        int line_start_col = 0;
        bool line_continues = true;
        
        while (line_continues && screen_lines_used < max_lines) {
            if (line_start_col == 0) {
                char line_num[8];
                int2str(line + 1, line_num, sizeof(line_num));
                draw_text(10, y_pos, line_num, VGA_COLOUR_DARK_GREY);
            }
            
            int chars_that_fit = 0;
            int pixel_width = 0;
            
            for (int i = line_start_col; i < line_length; i++) {
                char c = line_start[i];
                int char_width = char_widths[(unsigned char)c] + 1; // +1 for spacing
                
                if (pixel_width + char_width > editor_width) {
                    break;
                }
                
                pixel_width += char_width;
                chars_that_fit++;
            }
            
            if (chars_that_fit == 0 && line_start_col < line_length) {
                chars_that_fit = 1;
            }
            
            char line_buffer[256];
            int chars_to_draw = 0;
            
            for (int i = 0; i < chars_that_fit && (line_start_col + i) < line_length; i++) {
                line_buffer[chars_to_draw] = line_start[line_start_col + i];
                chars_to_draw++;
            }
            line_buffer[chars_to_draw] = '\0';
            
            // Draw line content - start after line numbers
            draw_text(EDITOR_LINE_NUM_WIDTH + 5, y_pos, line_buffer, VGA_COLOUR_BLACK);

            // Draw cursor if it's on this line segment
            if (line == editor_cursor_line) {
                int cursor_x = EDITOR_LINE_NUM_WIDTH + 5; // Start after line numbers
                if (editor_cursor_col >= line_start_col && 
                    editor_cursor_col < line_start_col + chars_that_fit) {
                    int cursor_col_in_segment = editor_cursor_col - line_start_col;
                    
                    // Calculate cursor position
                    for (int i = 0; i < cursor_col_in_segment && i < chars_to_draw; i++) {
                        cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1;
                    }
                    
                    draw_line_vertical(cursor_x, y_pos, y_pos + 8, VGA_COLOUR_BLUE);
                }
                // If cursor is at the end of the line, we move the cursor to the next line
                else if (editor_cursor_col == line_start_col + chars_that_fit && 
                         line_start_col + chars_that_fit >= line_length) {
                    for (int i = 0; i < chars_to_draw; i++) {
                        cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1;
                    }
                    draw_line_vertical(cursor_x, y_pos, y_pos + 8, VGA_COLOUR_BLUE);
                }
            }
            
            line_start_col += chars_that_fit;
            y_pos += EDITOR_LINE_HEIGHT;
            screen_lines_used++;
            
            if (line_start_col >= line_length) {
                line_continues = false;
            }
            
            if (chars_that_fit == 0) {
                line_continues = false;
            }
        }
    }

    // Draw status bar
    draw_rect(0, 190, SCREEN_WIDTH, 10, VGA_COLOUR_DARK_GREY);

    // Show cursor/line info and scroll position
    char status[64] = "Line: ";
    char line_str[8], col_str[8], scroll_str[8];
    int2str(editor_cursor_line + 1, line_str, sizeof(line_str));
    int2str(editor_cursor_col + 1, col_str, sizeof(col_str));
    int2str(editor_scroll_line + 1, scroll_str, sizeof(scroll_str));
    
    strcat(status, line_str);
    strcat(status, ", Col: ");
    strcat(status, col_str);
    
    draw_text(5, SCREEN_HEIGHT - 8, status, VGA_COLOUR_WHITE); // Bottom of screen
}



/**
 * Open text editor
 */
void editor_open(const char* filename) {
    // Copy filename
    copyStr(editor_filename, filename);

    // Get content
    char* content = get_file_content(filename);
    if (content) {
        // Copy content to buffer
        int i = 0;
        while (content[i] && i < MAX_CONTENT - 1) {
            editor_content[i] = content[i];
            i++;
        }
        // End file
        editor_content[i] = '\0';
    } else {
        // File is empty
        editor_content[0] = '\0';
    }

    // Reset editor state
    editor_cursor_pos = 0;
    editor_scroll_line = 0;
    cursorpos2linecol(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
    editor_modified = false;

    // Activate editor, deactivate explorer, etc.
    editor_active = true;
    explorer_active = false;
    terminal_active = false;
    dialog_active = false;

    extern void gui_clearmouse(void);
    gui_clearmouse();
    // Draw editor
    editor_draw();
}

/**
 * Calculate which screen line the cursor is on
 */
int calculate_cursor_screen_line() {
    int screen_line = 0;
    int editor_width = EDITOR_WIDTH - EDITOR_LINE_NUM_WIDTH - 10; // Proportional width
    
    for (int line = 0; line < editor_cursor_line; line++) {
        const char* line_start = get_line_start(editor_content, line);
        int line_length = len_line(line_start);
        
        if (line_length == 0) {
            screen_line++;
            continue;
        }
        
        int line_start_col = 0;
        while (line_start_col < line_length) {
            int chars_that_fit = 0;
            int pixel_width = 0;
            
            for (int i = line_start_col; i < line_length; i++) {
                char c = line_start[i];
                int char_width = char_widths[(unsigned char)c] + 1;
                
                if (pixel_width + char_width > editor_width) {
                    break;
                }
                
                pixel_width += char_width;
                chars_that_fit++;
            }
            
            if (chars_that_fit == 0) chars_that_fit = 1; 
            
            screen_line++;
            line_start_col += chars_that_fit;
        }
    }
    
    const char* cursor_line_start = get_line_start(editor_content, editor_cursor_line);
    int cursor_line_length = len_line(cursor_line_start);
    
    if (cursor_line_length == 0) {
        return screen_line;
    }
    
    int line_start_col = 0;
    while (line_start_col < cursor_line_length) {
        int chars_that_fit = 0;
        int pixel_width = 0;
        
        for (int i = line_start_col; i < cursor_line_length; i++) {
            char c = cursor_line_start[i];
            int char_width = char_widths[(unsigned char)c] + 1;
            
            if (pixel_width + char_width > editor_width) {
                break;
            }
            
            pixel_width += char_width;
            chars_that_fit++;
        }
        
        if (chars_that_fit == 0) chars_that_fit = 1;
        
        if (editor_cursor_col >= line_start_col && 
            editor_cursor_col <= line_start_col + chars_that_fit) {
            return screen_line;
        }
        
        screen_line++;
        line_start_col += chars_that_fit;
    }
    
    return screen_line;
}

/**
 * Calculate total screen lines used by content
 */
int calculate_total_screen_lines() {
    int screen_lines = 0;
    int total_lines = count_lines(editor_content);
    int editor_width = EDITOR_WIDTH - EDITOR_LINE_NUM_WIDTH - 10; // Proportional width
    
    for (int line = 0; line < total_lines; line++) {
        const char* line_start = get_line_start(editor_content, line);
        int line_length = len_line(line_start);
        
        if (line_length == 0) {
            screen_lines++;
            continue;
        }
        
        int line_start_col = 0;
        while (line_start_col < line_length) {
            int chars_that_fit = 0;
            int pixel_width = 0;
            
            for (int i = line_start_col; i < line_length; i++) {
                char c = line_start[i];
                int char_width = char_widths[(unsigned char)c] + 1;
                
                if (pixel_width + char_width > editor_width) {
                    break;
                }
                
                pixel_width += char_width;
                chars_that_fit++;
            }
            
            if (chars_that_fit == 0) chars_that_fit = 1;
            
            screen_lines++;
            line_start_col += chars_that_fit;
        }
    }
    
    return screen_lines;
}

/**
 * Ensure cursor is visible
 */
void editor_cursor_visible() {
    // Don't scroll if we're at the very beginning
    if (editor_cursor_line == 0 && editor_cursor_col == 0 && editor_scroll_line == 0) {
        return;
    }
    
    int max_lines = EDITOR_LINES_VISIBLE; 
    int cursor_screen_line = calculate_cursor_screen_line();
    
    int scroll_screen_line = 0;
    int editor_width = EDITOR_WIDTH - EDITOR_LINE_NUM_WIDTH - 10;
    
    for (int line = 0; line < editor_scroll_line; line++) {
        const char* line_start = get_line_start(editor_content, line);
        int line_length = len_line(line_start);
        
        if (line_length == 0) {
            scroll_screen_line++;
            continue;
        }
        
        int line_start_col = 0;
        while (line_start_col < line_length) {
            int chars_that_fit = 0;
            int pixel_width = 0;
            
            for (int i = line_start_col; i < line_length; i++) {
                char c = line_start[i];
                int char_width = char_widths[(unsigned char)c] + 1;
                
                if (pixel_width + char_width > editor_width) {
                    break;
                }
                
                pixel_width += char_width;
                chars_that_fit++;
            }
            
            if (chars_that_fit == 0) chars_that_fit = 1;
            
            scroll_screen_line++;
            line_start_col += chars_that_fit;
        }
    }
    
    if (cursor_screen_line < scroll_screen_line) {
        // Cursor is above visible area, scroll up
        while (cursor_screen_line < scroll_screen_line && editor_scroll_line > 0) {
            editor_scroll_line--;
            scroll_screen_line = 0;
            for (int line = 0; line < editor_scroll_line; line++) {
                const char* line_start = get_line_start(editor_content, line);
                int line_length = len_line(line_start);
                
                if (line_length == 0) {
                    scroll_screen_line++;
                    continue;
                }
                
                int line_start_col = 0;
                while (line_start_col < line_length) {
                    int chars_that_fit = 0;
                    int pixel_width = 0;
                    
                    for (int i = line_start_col; i < line_length; i++) {
                        char c = line_start[i];
                        int char_width = char_widths[(unsigned char)c] + 1;
                        
                        if (pixel_width + char_width > editor_width) {
                            break;
                        }
                        
                        pixel_width += char_width;
                        chars_that_fit++;
                    }
                    
                    if (chars_that_fit == 0) chars_that_fit = 1;
                    
                    scroll_screen_line++;
                    line_start_col += chars_that_fit;
                }
            }
        }
    } else if (cursor_screen_line >= scroll_screen_line + max_lines) {
        // Cursor is below visible area, scroll down
        while (cursor_screen_line >= scroll_screen_line + max_lines) {
            editor_scroll_line++;
            scroll_screen_line = 0;
            for (int line = 0; line < editor_scroll_line; line++) {
                const char* line_start = get_line_start(editor_content, line);
                int line_length = len_line(line_start);
                
                if (line_length == 0) {
                    scroll_screen_line++;
                    continue;
                }
                
                int line_start_col = 0;
                while (line_start_col < line_length) {
                    int chars_that_fit = 0;
                    int pixel_width = 0;
                    
                    for (int i = line_start_col; i < line_length; i++) {
                        char c = line_start[i];
                        int char_width = char_widths[(unsigned char)c] + 1;
                        
                        if (pixel_width + char_width > editor_width) {
                            break;
                        }
                        
                        pixel_width += char_width;
                        chars_that_fit++;
                    }
                    
                    if (chars_that_fit == 0) chars_that_fit = 1;
                    
                    scroll_screen_line++;
                    line_start_col += chars_that_fit;
                }
            }
        }
    }
    
    if (editor_scroll_line < 0) {
        editor_scroll_line = 0;
    }
    int total_lines = count_lines(editor_content);
    if (editor_scroll_line >= total_lines) {
        editor_scroll_line = total_lines - 1;
        if (editor_scroll_line < 0) editor_scroll_line = 0;
    }
}

// Convert a cursor position to the line column
void cursorpos2linecol(int pos, int* line, int* col) {
    *line = 0;  // Reset to 0
    *col = 0;   // Reset to 0
    
    // Bounds check
    if (pos < 0) pos = 0;
    if (pos > strlen(editor_content)) pos = strlen(editor_content);
    
    for (int i = 0; i < pos && editor_content[i]; i++) {
        if (editor_content[i] == '\n') {
            (*line)++;
            *col = 0;  // Reset column on new line
        } else {
            (*col)++;
        }
    }
}

// Convert a line column to cursor position
int linecol2cursorpos(int line, int col) {
    int pos = 0;
    int current_line = 0;
    
    // Bounds check
    if (line < 0) line = 0;
    if (col < 0) col = 0;
    
    // Find the start of the target line
    while (current_line < line && editor_content[pos]) {
        if (editor_content[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    // Move to the target column within the line
    int current_col = 0;
    while (current_col < col && editor_content[pos] && editor_content[pos] != '\n') {
        pos++;
        current_col++;
    }
    
    return pos;
}