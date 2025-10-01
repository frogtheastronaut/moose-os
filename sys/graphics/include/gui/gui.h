/*
    MooseOS GUI code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
#ifndef GUI_H
#define GUI_H

#include "vga/vga.h"
#include "icons/icons.h"
#include "fontdef/fontdef.h"
#include "keyboard/scancode_map.h"
#include "file/file.h"
#include "keyboard/keyboard_scan_codes.h"
#include "libc/lib.h"
#include "heap/heap.h"

// type definitions
typedef unsigned short uint16_t;
typedef short int16_t;

// the screen width and height
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// the current selection for files/folders
// (used in explorer.c)
extern int current_selection;

// max dialog input length.
/** @todo: maybe turn this down a bit? 128 characters is a bit long. */
#define MAX_DIALOG_INPUT_LEN 128

// cursor tracking structure
typedef struct {
    int x, y;
    uint8_t original_colour;
    bool is_modified;
} cursor_pixel;

// external variables
extern uint8_t* vga_buffer;
extern int last_mouse_x;
extern int last_mouse_y;

// booleans for active apps
extern bool dialog_active;
extern bool explorer_active;
extern char dialog_input[MAX_DIALOG_INPUT_LEN + 1];
extern int dialog_input_pos;
extern int dialog_type;

// editor variables
extern bool editor_active;
extern char editor_content[MAX_CONTENT];
extern char editor_filename[MAX_NAME_LEN];
extern int editor_cursor_pos;
extern int editor_scroll_line;
extern int editor_cursor_line;
extern int editor_cursor_col;
extern bool editor_modified;

// editor definitions.
#define EDITOR_LINES_VISIBLE 14  // full screen minus title bar and some padding
#define EDITOR_LINE_HEIGHT 12
#define EDITOR_CHAR_WIDTH 8
#define EDITOR_START_X 0
#define EDITOR_START_Y 20
#define EDITOR_WIDTH SCREEN_WIDTH
#define EDITOR_LINE_NUM_WIDTH 45 

// GUI functions
void update_mouse(void);
void gui_update_mouse(void);
void draw_cursor(void);
void draw_window_box(int x, int y, int width, int height, uint8_t outer_colour, uint8_t inner_colour, uint8_t face_colour);
void draw_title(int x, int y, int width, int title_height, uint8_t title_colour);
void gui_set_pixel(int x, int y, uint8_t colour);
void draw_line_horizontal(int x1, int x2, int y, uint8_t colour);
void gui_open_terminal(void);
void gui_clear(uint8_t colour);
void draw_rect(int x, int y, int width, int height, uint8_t colour);
void draw_text(int x, int y, const char* text, uint8_t colour);
int draw_text_width(const char* text);
void gui_init(void);
void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t colour, uint8_t bg_colour);
void draw_line_vertical(int x, int y1, int y2, uint8_t colour);
int count_lines(const char* text);
const char* get_line_start(const char* text, int line_num);
int len_line(const char* line_start);
char* get_file_content(const char* filename);
void cursorpos2linecol(int pos, int* line, int* col);
void draw_file(int x, int y, const char* name, int is_dir, int is_selected);
int linecol2cursorpos(int line, int col);

#endif