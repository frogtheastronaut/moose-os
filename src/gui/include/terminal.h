/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for ../terminal.c
*/
#ifndef TERMINAL_H 
#define TERMINAL_H

// Includes
#include "gui.h"
#include "../../drivers/include/keyboard_scan_codes.h"
#include "../../drivers/include/scancode_map.h"
#include "../../filesys/include/file.h"
#include "../../lib/include/lib.h"
#include "../../time/include/rtc.h"

// Definitions
#define TERMINAL_WIDTH SCREEN_WIDTH
#define TERMINAL_HEIGHT SCREEN_HEIGHT
#define TERMINAL_X 0
#define TERMINAL_Y 0

// Display area - full screen without title bar
#define TERM_AREA_X 0
#define TERM_AREA_Y 0  
#define TERM_AREA_WIDTH TERMINAL_WIDTH
#define TERM_AREA_HEIGHT TERMINAL_HEIGHT

// Colours
#define TERM_BG_COLOR VGA_COLOR_BLACK
#define TERM_TEXT_COLOR VGA_COLOR_WHITE
#define TERM_PROMPT_COLOR VGA_COLOR_GREEN
#define TERM_ERROR_COLOR VGA_COLOR_RED

// Terminal settings
#define MAX_COMMAND_LEN 64
#define MAX_LINES 20
#define CHARS_PER_LINE 50
#define FONT_HEIGHT 10
#define FONT_SPACING 5

// Boolean definition
#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif

// External functions
extern void copyStr(char* dest, const char* src);
extern int strEqual(const char* a, const char* b);
extern int msnprintf(char *buffer, int size, const char *format, ...);
extern char* strcat(char* dest, const char* src);
extern const char* strip_whitespace(const char* str);
extern void dock_return(void); 
extern void draw_cursor(void);
extern const char* get_cwd();
extern void terminal_print_error(const char* text);
extern void terminal_add_wrapped_text(const char* text, uint8_t color);
extern void clear_terminal(void);

// External variables
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern File* root;
extern File* cwd;

// Function prototypes
bool term_isactive(void);
void terminal_print(const char* text);


#endif