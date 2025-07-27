/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/gui.h"

extern bool terminal_active;
void editor_ensure_cvis();

/**
 * draw editor
 */
void editor_draw() {
    // clear
    draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, VGA_COLOR_LIGHT_GREY);
    
    // title bar
    draw_title(0, 0, SCREEN_WIDTH, 15, VGA_COLOR_BLUE);
    
    // label on title bar
    char label[] = "Text Editor - ";
    draw_text(10, 6, label, VGA_COLOR_WHITE);
    
    //file name, scroll if to BIG
    int label_width = get_textwidth(label);
    int filename_x = 10 + label_width;
    int max_filename_width = SCREEN_WIDTH - filename_x - 10;
    draw_text_scroll(filename_x, 6, editor_filename, max_filename_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);

    
    // draw numbers
    draw_rect(10, 40, 25, 150, VGA_COLOR_LIGHT_GREY);
    draw_line_vert(35, 40, 190, VGA_COLOR_DARK_GREY);
    
    // draw content with scrolling
    int y_pos = EDITOR_START_Y + 5;
    int max_lines = EDITOR_LINES_VISIBLE - 3;  
    int total_lines = count_lines(editor_content);
    int editor_width = EDITOR_WIDTH - 50; 
    

    if (editor_scroll_line >= total_lines) {
        editor_scroll_line = total_lines - 1;
        if (editor_scroll_line < 0) editor_scroll_line = 0;
    }
    
    editor_ensure_cvis();
    
    // draw visible lines with wrapping
    int visible_line_count = 0;
    int screen_lines_used = 0;  
    
    for (int line = editor_scroll_line; line < total_lines && screen_lines_used < max_lines; line++) {
        // get line content
        const char* line_start = get_line_start(editor_content, line);
        int line_length = len_line(line_start);
        
        if (line_length == 0) {
            // draw line number
            char line_num[8];
            int2str(line + 1, line_num, sizeof(line_num));
            draw_text(12, y_pos, line_num, VGA_COLOR_DARK_GREY);
            
            // draw cursor if it's on this empty line
            if (line == editor_cursor_line && editor_cursor_col == 0) {
                int cursor_x = EDITOR_START_X + 25;
                draw_line_vert(cursor_x, y_pos, y_pos + 8, VGA_COLOR_BLACK);
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
                draw_text(12, y_pos, line_num, VGA_COLOR_DARK_GREY);
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
            
            // draw line content
            draw_text(EDITOR_START_X + 25, y_pos, line_buffer, VGA_COLOR_BLACK);
            
            // draw cursor if it's on this line segment
            if (line == editor_cursor_line) {
                int cursor_x = EDITOR_START_X + 25;
                bool draw_cursor = false;
                
                if (editor_cursor_col >= line_start_col && 
                    editor_cursor_col <= line_start_col + chars_that_fit) {
                    int cursor_col_in_segment = editor_cursor_col - line_start_col;
                    
                    if (cursor_col_in_segment > chars_to_draw) {
                        cursor_col_in_segment = chars_to_draw;
                    }
                    
                    for (int i = 0; i < cursor_col_in_segment; i++) {
                        cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1;
                    }
                    draw_cursor = true;
                }
                
                if (draw_cursor) {
                    draw_line_vert(cursor_x, y_pos, y_pos + 8, VGA_COLOR_BLACK);
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
    
    // draw stat bar
    draw_rect(0, 190, SCREEN_WIDTH, 10, VGA_COLOR_DARK_GREY);
    
    // show cursor/line info and scroll position
    char status[64] = "Line: ";
    char line_str[8], col_str[8], scroll_str[8];
    int2str(editor_cursor_line + 1, line_str, sizeof(line_str));
    int2str(editor_cursor_col + 1, col_str, sizeof(col_str));
    int2str(editor_scroll_line + 1, scroll_str, sizeof(scroll_str));
    
    strcat(status, line_str);
    strcat(status, ", Col: ");
    strcat(status, col_str);
    
    draw_text(5, 192, status, VGA_COLOR_WHITE);
}



/**
 * open text editorrrrrrrrrrrrrrr (sry)
 */
void editor_open(const char* filename) {
    // copy filename 
    copyStr(editor_filename, filename);
    
    // get content
    char* content = get_file_content(filename);
    if (content) {
        // copy content to editor buffer thingy
        int i = 0;
        while (content[i] && i < MAX_CONTENT - 1) {
            editor_content[i] = content[i];
            i++;
        }
        // end file
        editor_content[i] = '\0';
    } else {
        // nope, file is empty
        editor_content[0] = '\0';
    }
    
    // reset editor state
    editor_cursor_pos = 0;
    editor_scroll_line = 0;
    cursorpos2linecol(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
    editor_modified = false;
    
    // activate editor, deactivate explorer and stuff
    editor_active = true;
    explorer_active = false;
    terminal_active = false;
    dialog_active = false;

    extern void gui_clearmouse(void);
    gui_clearmouse();
    // draw editor (nah rlly?)
    editor_draw();
}

/**
 * calculate which screen line the cursor is on
 */
int calculate_cursor_screen_line() {
    int screen_line = 0;
    int editor_width = EDITOR_WIDTH - 50;
    
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
 * calculate total screen lines used by content
 */
int calculate_total_screen_lines() {
    int screen_lines = 0;
    int total_lines = count_lines(editor_content);
    int editor_width = EDITOR_WIDTH - 50; 
    
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
 * ensure cursor is visible
 */
void editor_ensure_cvis() {
    int max_lines = EDITOR_LINES_VISIBLE - 3;  
    int cursor_screen_line = calculate_cursor_screen_line();
    
    int scroll_screen_line = 0;
    int editor_width = EDITOR_WIDTH - 50;
    
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
        // cursor is above visible area, scroll up
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
        // cursor is below visible area, scroll down
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