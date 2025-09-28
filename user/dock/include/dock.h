/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for ../dock.c
*/

#ifndef DOCK_H 
#define DOCK_H 

// Includes
#include "vga/vga.h"
#include "heap/heap.h"
#include "task/task.h"
#include "icons/icons.h"
#include "keyboard/scancode_map.h"
#include "file/file.h"
#include "keyboard/keyboard_scan_codes.h"
#include "libc/lib.h"
#include "terminal.h"
#include "rtc/rtc.h"
#include "mouse/mouse.h"

// Definitions
#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif

// Types
typedef unsigned short uint16_t;
typedef short int16_t;

// External functions
extern void editor_open(const char* filename);
extern void draw_explorer();
extern void draw_cursor(void);
extern void gui_clearmouse(void);

// Window height, width, and position
// Window height/width is the screen height/width
/**
 * @todo Add windowed apps
 * This should be high priority
 */
#define WINDOW_WIDTH SCREEN_WIDTH    
#define WINDOW_HEIGHT SCREEN_HEIGHT   
#define WINDOW_X 0
#define WINDOW_Y 0

// Height of title bar
#define TITLE_BAR_HEIGHT 20

// Size of file area
#define FILE_AREA_X (WINDOW_X + 8)
#define FILE_AREA_Y (WINDOW_Y + TITLE_BAR_HEIGHT + 8)
#define FILE_AREA_WIDTH (WINDOW_WIDTH - 16)
#define FILE_AREA_HEIGHT (WINDOW_HEIGHT - TITLE_BAR_HEIGHT - 16)

// File display
#define ICON_SIZE 20
#define FILE_SPACING_X 80    // Horizontal spacing between files
#define FILE_SPACING_Y 60    // Vertical spacing between files
#define FILES_PER_ROW 3      // Amount of Files per row

// Colours
#define WINDOW_BACKGROUND VGA_COLOUR_LIGHT_GREY
#define WINDOW_BORDER_OUTER VGA_COLOUR_BLACK
#define WINDOW_BORDER_INNER VGA_COLOUR_WHITE
#define TITLE_BAR_COLOUR VGA_COLOUR_BLUE
#define TITLE_TEXT_COLOUR VGA_COLOUR_WHITE
#define FILE_TEXT_COLOUR VGA_COLOUR_BLACK
#define SELECTION_COLOUR VGA_COLOUR_BLUE
#define SELECTION_TEXT_COLOUR VGA_COLOUR_WHITE

// External variables
extern bool dialog_active;
extern char dialog_input[129];
extern int dialog_input_pos;
extern int dialog_type;
extern void draw_dialog(const char* title, const char* prompt);
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern volatile uint32_t ticks;
extern File* root;
extern File* cwd;
extern int filesystem_make_file(const char* name, const char* content);

// Functions
void dock_mkopen_file(void);
bool dock_handle_mouse(void);
void dock_update_time(void);
void dock_init(void);

// For the New File dialog.
// While this variable is a bit vague, you can check dock.c and explorer.c to see its usage.
#define DIALOG_TYPE_NEW_FILE 2
#endif