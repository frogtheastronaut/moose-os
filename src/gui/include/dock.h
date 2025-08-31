#ifndef DOCK_H 
#define DOCK_H 

#include "../../kernel/include/vga.h"
#include "../../kernel/include/mouse.h"
#include "../../kernel/include/task.h"
#include "images.h"
#include "../../kernel/include/keydef.h"
#include "../../filesys/file.h"
#include "../../kernel/include/keyboard.h"
#include "../../lib/lib.h"
#include "terminal.h"
#include "../../time/rtc.h"

// defines
#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif
typedef unsigned short uint16_t;
typedef short int16_t;

// externs
extern void editor_open(const char* filename);
extern void draw_filesplorer();
extern void draw_cursor(void);
extern void gui_clearmouse(void);

// interns (badum tsss)
#define WINDOW_WIDTH SCREEN_WIDTH    
#define WINDOW_HEIGHT SCREEN_HEIGHT   
#define WINDOW_X 0
#define WINDOW_Y 0

// title bar height lmao
#define TITLE_BAR_HEIGHT 20

// file area
#define FILE_AREA_X (WINDOW_X + 8)
#define FILE_AREA_Y (WINDOW_Y + TITLE_BAR_HEIGHT + 8)
#define FILE_AREA_WIDTH (WINDOW_WIDTH - 16)
#define FILE_AREA_HEIGHT (WINDOW_HEIGHT - TITLE_BAR_HEIGHT - 16)

// file display
#define ICON_SIZE 20         // Same as file explorer
#define FILE_SPACING_X 80    // Horizontal spacing between files
#define FILE_SPACING_Y 60    // Vertical spacing between files
#define FILES_PER_ROW 3      // How many files per row

// colors (colours, im australién)
#define WINDOW_BACKGROUND VGA_COLOR_LIGHT_GREY
#define WINDOW_BORDER_OUTER VGA_COLOR_BLACK
#define WINDOW_BORDER_INNER VGA_COLOR_WHITE
#define TITLE_BAR_COLOR VGA_COLOR_BLUE
#define TITLE_TEXT_COLOR VGA_COLOR_WHITE
#define FILE_TEXT_COLOR VGA_COLOR_BLACK
#define SELECTION_COLOR VGA_COLOR_BLUE
#define SELECTION_TEXT_COLOR VGA_COLOR_WHITE

// extern variables
extern bool dialog_active;
extern char dialog_input[129];
extern int dialog_input_pos;
extern int dialog_type;
extern void draw_dialog(const char* title, const char* prompt);
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern volatile uint32_t ticks;

// more
extern File* root;
extern File* cwd;
extern int filesys_mkfile(const char* name, const char* content);

void dock_mkopen_file(void);
bool dock_handle_mouse(void);
void dock_update_time(void);
void dock_init(void);

// huh
#define DIALOG_TYPE_NEW_FILE 2

#endif