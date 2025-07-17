/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/gui.h"

extern bool terminal_active;

/**
 * Draws the text editor interface
 */
void editor_draw() {
    // clear
    gui_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, VGA_COLOR_LIGHT_GREY);
    
    // title bar
    gui_draw_title_bar(0, 0, SCREEN_WIDTH, 15, VGA_COLOR_BLUE);
    
    // label on title bar
    char label[] = "Text Editor - ";
    gui_draw_text(10, 6, label, VGA_COLOR_WHITE);
    
    //file name, scroll if to BIG
    int label_width = gui_text_width(label);
    int filename_x = 10 + label_width;
    int max_filename_width = SCREEN_WIDTH - filename_x - 10; // Leave 10px margin on right
    gui_draw_scrolling_text(filename_x, 6, editor_filename, max_filename_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);

    
    // draw numbers
    gui_draw_rect(10, 40, 25, 150, VGA_COLOR_LIGHT_GREY);
    gui_draw_vline(35, 40, 190, VGA_COLOR_DARK_GREY);
    
    // draw content
    int y_pos = EDITOR_START_Y + 5;
    int max_lines = EDITOR_LINES_VISIBLE;
    int total_lines = count_lines(editor_content);
    
    // draw visible lines
    for (int line = 0; line < max_lines && (editor_scroll_line + line) < total_lines; line++) {
        int actual_line = editor_scroll_line + line;
        
        // draw line number
        char line_num[8];
        int_to_str(actual_line + 1, line_num, sizeof(line_num));
        gui_draw_text(12, y_pos, line_num, VGA_COLOR_DARK_GREY);
        
        // get line content
        const char* line_start = get_line_start(editor_content, actual_line);
        int line_length = get_line_length(line_start);
        
        // copy chars
        char line_buffer[EDITOR_MAX_CHARS_PER_LINE + 1];
        int chars_to_draw = 0;
        for (int i = 0; i < EDITOR_MAX_CHARS_PER_LINE && i < line_length; i++) {
            line_buffer[chars_to_draw] = line_start[i];
            chars_to_draw++;
        }
        line_buffer[chars_to_draw] = '\0';
        
        // draw line content
        gui_draw_text(EDITOR_START_X + 25, y_pos, line_buffer, VGA_COLOR_BLACK);
        
        // draw cursor
        if (actual_line == editor_cursor_line) {
            // get cursor x
            int cursor_x = EDITOR_START_X + 25;
            int cursor_col_to_show = editor_cursor_col;
            
            for (int i = 0; i < cursor_col_to_show && i < chars_to_draw; i++) {
                cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1; // +1 for character spacing
            }
            gui_draw_vline(cursor_x, y_pos, y_pos + 10, VGA_COLOR_BLACK);
        }
        
        y_pos += EDITOR_LINE_HEIGHT;
    }
    
    // draw stat bar
    gui_draw_rect(0, 190, SCREEN_WIDTH, 10, VGA_COLOR_DARK_GREY);
    
    // who cursor/line
    char status[64] = "Line: ";
    char line_str[8], col_str[8];
    int_to_str(editor_cursor_line + 1, line_str, sizeof(line_str));
    int_to_str(editor_cursor_col + 1, col_str, sizeof(col_str));
    strcat(status, line_str);
    strcat(status, ", Col: ");
    strcat(status, col_str);
    
    gui_draw_text(5, 192, status, VGA_COLOR_WHITE);
    
    // Make sure cursor is visible after redraw
    cursor_visible = false;
    gui_update_mouse_cursor();
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
    cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
    editor_modified = false;
    
    // activate editor, deactivate explorer and stuff
    editor_active = true;
    explorer_active = false;
    terminal_active = false;
    dialog_active = false;
    
    // draw editor (nah rlly?)
    editor_draw();
}