#ifndef GUI_H
#define GUI_H

#include "../../kernel/include/vga.h"
#include "images.h"
#include "fontdef.h"
#include "../../kernel/include/keydef.h"
#include "../../filesys/file.h"
#include "../../kernel/include/keyboard.h" 
#include "../../lib/lib.h"
#include "../../kernel/include/mouse.h"

#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif


typedef unsigned short uint16_t;
typedef short int16_t;

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

extern int current_selection;

#define MAX_DIALOG_INPUT_LEN 128
extern bool dialog_active;
extern bool explorer_active;
extern char dialog_input[MAX_DIALOG_INPUT_LEN + 1];
extern int dialog_input_pos;
extern int dialog_type;

extern bool editor_active;
extern char editor_content[MAX_CONTENT];
extern char editor_filename[MAX_NAME_LEN];
extern int editor_cursor_pos;
extern int editor_scroll_line;
extern int editor_cursor_line;
extern int editor_cursor_col;
extern bool editor_modified;

#define EDITOR_LINES_VISIBLE 14  // Full screen minus title and status bar
#define EDITOR_LINE_HEIGHT 12
#define EDITOR_CHAR_WIDTH 8
#define EDITOR_START_X 0
#define EDITOR_START_Y 20
#define EDITOR_WIDTH SCREEN_WIDTH
#define EDITOR_LINE_NUM_WIDTH 45  // Proportional line number area


void update_mouse(void);
void gui_updatemouse(void);
void draw_cursor(void);

void draw_windowbox(int x, int y, int width, int height, uint8_t outer_color, uint8_t inner_color, uint8_t face_color);
void draw_title(int x, int y, int width, int title_height, uint8_t title_color);
void gui_set_pixel(int x, int y, uint8_t color);
void draw_line_hrzt(int x1, int x2, int y, uint8_t color);
void gui_open_terminal(void);

void gui_clear(uint8_t color);
void draw_rect(int x, int y, int width, int height, uint8_t color);
void draw_text(int x, int y, const char* text, uint8_t color);
int get_textwidth(const char* text);
void gui_init(void);

void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t color, uint8_t bg_color);
void draw_line_vert(int x, int y1, int y2, uint8_t color);
int count_lines(const char* text);
const char* get_line_start(const char* text, int line_num);
int len_line(const char* line_start);
char* get_file_content(const char* filename);
void cursorpos2linecol(int pos, int* line, int* col);

void draw_file(int x, int y, const char* name, int is_dir, int is_selected);
int linecol2cursorpos(int line, int col);

#endif