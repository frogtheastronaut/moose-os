/*
    MooseOS Terminal
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#ifndef TERMINAL_H 
#define TERMINAL_H

#include "gui/gui.h"
#include "keyboard/keyboard_scan_codes.h"
#include "keyboard/scancode_map.h"
#include "file/file.h"
#include "libc/lib.h"
#include "rtc/rtc.h"

#define TERMINAL_WIDTH SCREEN_WIDTH
#define TERMINAL_HEIGHT SCREEN_HEIGHT
#define TERM_AREA_X 0
#define TERM_AREA_Y 0  

#define TERM_BG_COLOUR VGA_COLOUR_BLACK
#define TERM_TEXT_COLOUR VGA_COLOUR_WHITE
#define TERM_PROMPT_COLOUR VGA_COLOUR_GREEN
#define TERM_ERROR_COLOUR VGA_COLOUR_RED

#define MAX_COMMAND_LEN 64
#define MAX_LINES 20
#define CHARS_PER_LINE 50
#define FONT_HEIGHT 10
#define FONT_SPACING 5

// external functions
extern void strcpy(char* dest, const char* src);
extern int strcmp(const char* a, const char* b);
extern int msnprintf(char *buffer, int size, const char *format, ...);
extern char* strcat(char* dest, const char* src);
extern const char* strip_whitespace(const char* str);
extern void dock_return(void); 
extern void draw_cursor(void);
extern const char* get_cwd();
extern void terminal_print_error(const char* text);
extern void terminal_add_wrapped_text(const char* text, uint8_t colour);
extern void clear_terminal(void);

// external variables
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern File* root;
extern File* cwd;

// function prototypes
bool term_isactive(void);
void terminal_print(const char* text);


#endif // TERMINAL_H
