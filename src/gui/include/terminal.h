#ifndef TERMINAL_H 
#define TERMINAL_H

// Includes
#include "gui.h"
#include "../../kernel/include/keyboard.h"
#include "../../kernel/include/keydef.h"
#include "../../filesys/file.h"
#include "../../lib/lib.h"
#include "../../time/rtc.h"

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
#define MAX_LINES 15
#define CHARS_PER_LINE 35

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