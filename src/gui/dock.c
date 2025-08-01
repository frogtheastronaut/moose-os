/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

// lots and lots of includes
#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "../kernel/include/mouse.h"
#include "../kernel/include/task.h"
#include "include/images.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h"
#include "../lib/lib.h"
#include "include/terminal.h"
#include "../time/rtc.h"

// defines

// externs
extern void editor_open(const char* filename);
extern void draw_filesplorer();
extern void draw_cursor(void);
extern void gui_clearmouse(void);

// interns (badum tsss)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200


#define WINDOW_WIDTH 300    
#define WINDOW_HEIGHT 180   
#define WINDOW_X ((SCREEN_WIDTH - WINDOW_WIDTH) / 2)
#define WINDOW_Y ((SCREEN_HEIGHT - WINDOW_HEIGHT) / 2)

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

// vars
static int selected_app = 0;  // 0 = File Explorer, 1 = Text Editor, 2 = Terminal
static const int total_apps = 3;  
static uint32_t last_time_update = 0;  // last update
static char last_time_str[32] = "";    // last time as in time time

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

// func decs
static void launch_selected_app(void);
static void handle_shutdown(void);
static void dock_mkopen_file(void);
static bool dock_handle_mouse_click(int mouse_x, int mouse_y);

// huh
#define DIALOG_TYPE_NEW_FILE 2


/**
 * draw file explorer
 */
static void draw_windowborder() {
    draw_windowbox(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, 
                       VGA_COLOR_BLACK,
                       VGA_COLOR_WHITE,
                       VGA_COLOR_LIGHT_GREY);
    
    draw_title(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, 15, VGA_COLOR_BLUE);
    draw_text(WINDOW_X + 5, WINDOW_Y + 3, "Welcome to MooseOS", VGA_COLOR_WHITE);
}

/**
 * draw icon
 */
static void dock_draw_icon(int x, int y, uint8_t bg_color, int app_index) {
    const uint8_t (*icon_bitmap)[16];
    
    // app is...
    switch (app_index) {
        case 0: // fexploprer
            icon_bitmap = folder_icon;
            break;
        case 2: // temriasndfal
            icon_bitmap = terminal_icon;
            break;
        case 1: // ted
        default:
            icon_bitmap = file_icon;
            break;
    }
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint8_t pixel = icon_bitmap[row][col];
            
            if (pixel == 0) {
                if (bg_color == WINDOW_BACKGROUND) {
                    continue;
                }
                gui_set_pixel(x + col, y + row, bg_color);
            } else {
                gui_set_pixel(x + col, y + row, icon_color_map[pixel]);
            }
        }
    }
}

/**
 * draw app
 */
static void dock_draw_app(int app_index, const char* filename, int x, int y) {
    bool is_selected = (app_index == selected_app);
    
    // selection area
    int text_width = get_textwidth(filename);
    // commented out uses text width for selection_width
    // int selection_width = (text_width > ICON_SIZE) ? text_width : ICON_SIZE;
    int selection_width = 70;
    int selection_height = ICON_SIZE + 12; // Icon + text height
    
    // selection rectangle
    int selection_x = x - (selection_width - ICON_SIZE) / 2;
    int selection_y = y;
    
    // selection background
    if (is_selected) {
        // im blue
        draw_rect(selection_x - 2, selection_y - 2, selection_width + 4, selection_height + 4, SELECTION_COLOR);
    }
    
    // icon has correct bg
    uint8_t icon_bg = is_selected ? SELECTION_COLOR : WINDOW_BACKGROUND;
    
    // draw icon
    dock_draw_icon(x, y, icon_bg, app_index);
    
    // draw filename
    int text_x = x + (ICON_SIZE / 2) - (text_width / 2);
    int text_y = y + ICON_SIZE + 2;
    
    uint8_t text_color = is_selected ? SELECTION_TEXT_COLOR : FILE_TEXT_COLOR;
    draw_text(text_x, text_y, filename, text_color);
}

/**
 * draw all files
 */
static void dock_draw_apps() {
    // start
    int start_x = FILE_AREA_X + 20;
    int start_y = FILE_AREA_Y;
    
    // filexpseort
    dock_draw_app(0, "File Explorer", start_x, start_y);
    
    // ted
    dock_draw_app(1, "Text Editor", start_x + FILE_SPACING_X, start_y);
    
    // terminal
    dock_draw_app(2, "Terminal", start_x + FILE_SPACING_X * 2, start_y);
}

/**
 * time displaydsdf
 */
static void draw_time() {
    static rtc_time last_time = {0};
    
    // get current time
    rtc_time current_time = rtc_gettime();
    
    // Only redraw if seconds actually changed to reduce unnecessary redraws
    if (current_time.seconds == last_time.seconds && 
        current_time.minutes == last_time.minutes &&
        current_time.hours == last_time.hours &&
        current_time.day == last_time.day &&
        current_time.month == last_time.month &&
        current_time.year == last_time.year) {
        return; // No time change, skip expensive redraw
    }
    
    last_time = current_time;
    
    // time, in format HH:MM:SS DD/MM/YYYY UTC±X
    char time_str[32];
    time_str[0] = '0' + (current_time.hours / 10);
    time_str[1] = '0' + (current_time.hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (current_time.minutes / 10);
    time_str[4] = '0' + (current_time.minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (current_time.seconds / 10);
    time_str[7] = '0' + (current_time.seconds % 10);
    time_str[8] = ' ';
    
    // add date (DD/MM/YYYY format)
    time_str[9] = '0' + (current_time.day / 10);
    time_str[10] = '0' + (current_time.day % 10);
    time_str[11] = '/';
    time_str[12] = '0' + (current_time.month / 10);
    time_str[13] = '0' + (current_time.month % 10);
    time_str[14] = '/';
    time_str[15] = '2';
    time_str[16] = '0';
    time_str[17] = '0' + (current_time.year / 10);
    time_str[18] = '0' + (current_time.year % 10);
    
    // timezone - simplified
    int offset = timezone_offset; // from rtc.c
    time_str[19] = ' ';
    time_str[20] = 'U';
    time_str[21] = 'T';
    time_str[22] = 'C';
    if (offset >= 0) {
        time_str[23] = '+';
        if (offset < 10) {
            time_str[24] = '0' + offset;
            time_str[25] = '\0';
        } else {
            time_str[24] = '0' + (offset / 10);
            time_str[25] = '0' + (offset % 10);
            time_str[26] = '\0';
        }
    } else {
        int abs_offset = -offset;
        time_str[23] = '-';
        if (abs_offset < 10) {
            time_str[24] = '0' + abs_offset;
            time_str[25] = '\0';
        } else {
            time_str[24] = '0' + (abs_offset / 10);
            time_str[25] = '0' + (abs_offset % 10);
            time_str[26] = '\0';
        }
    }
    
    // status bar
    int status_bar_width = WINDOW_WIDTH - 16;
    int status_bar_height = 15;
    int status_bar_x = WINDOW_X + 8;
    int status_bar_y = WINDOW_Y + WINDOW_HEIGHT - status_bar_height - 8;
    
    // bar background
    draw_rect(status_bar_x, status_bar_y, status_bar_width, status_bar_height, VGA_COLOR_LIGHT_GREY);
    draw_line_hrzt(status_bar_x, status_bar_x + status_bar_width - 1, status_bar_y, VGA_COLOR_DARK_GREY);
    
    // text
    draw_text(status_bar_x + 5, status_bar_y + 3, time_str, VGA_COLOR_BLACK);
}

/*
    force dock redraw
*/
static void draw_time_forced() {
    static rtc_time last_time = {0};
    
    // get current time
    rtc_time current_time = rtc_gettime();

    
    last_time = current_time;
    
    // time, in format HH:MM:SS DD/MM/YYYY UTC±X
    char time_str[32];
    time_str[0] = '0' + (current_time.hours / 10);
    time_str[1] = '0' + (current_time.hours % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (current_time.minutes / 10);
    time_str[4] = '0' + (current_time.minutes % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (current_time.seconds / 10);
    time_str[7] = '0' + (current_time.seconds % 10);
    time_str[8] = ' ';
    
    // add date (DD/MM/YYYY format)
    time_str[9] = '0' + (current_time.day / 10);
    time_str[10] = '0' + (current_time.day % 10);
    time_str[11] = '/';
    time_str[12] = '0' + (current_time.month / 10);
    time_str[13] = '0' + (current_time.month % 10);
    time_str[14] = '/';
    time_str[15] = '2';
    time_str[16] = '0';
    time_str[17] = '0' + (current_time.year / 10);
    time_str[18] = '0' + (current_time.year % 10);
    
    // timezone - simplified
    int offset = timezone_offset; // from rtc.c
    time_str[19] = ' ';
    time_str[20] = 'U';
    time_str[21] = 'T';
    time_str[22] = 'C';
    if (offset >= 0) {
        time_str[23] = '+';
        if (offset < 10) {
            time_str[24] = '0' + offset;
            time_str[25] = '\0';
        } else {
            time_str[24] = '0' + (offset / 10);
            time_str[25] = '0' + (offset % 10);
            time_str[26] = '\0';
        }
    } else {
        int abs_offset = -offset;
        time_str[23] = '-';
        if (abs_offset < 10) {
            time_str[24] = '0' + abs_offset;
            time_str[25] = '\0';
        } else {
            time_str[24] = '0' + (abs_offset / 10);
            time_str[25] = '0' + (abs_offset % 10);
            time_str[26] = '\0';
        }
    }
    
    // status bar
    int status_bar_width = WINDOW_WIDTH - 16;
    int status_bar_height = 15;
    int status_bar_x = WINDOW_X + 8;
    int status_bar_y = WINDOW_Y + WINDOW_HEIGHT - status_bar_height - 8;
    
    // bar background
    draw_rect(status_bar_x, status_bar_y, status_bar_width, status_bar_height, VGA_COLOR_LIGHT_GREY);
    draw_line_hrzt(status_bar_x, status_bar_x + status_bar_width - 1, status_bar_y, VGA_COLOR_DARK_GREY);
    
    // text
    draw_text(status_bar_x + 5, status_bar_y + 3, time_str, VGA_COLOR_BLACK);
}

/**
 *  draw dock
 */
static void dock_draw_window() {
    // clear screen
    gui_clear(VGA_COLOR_LIGHT_GREY);
    
    // draw window
    draw_windowborder();
    
    // draw files
    dock_draw_apps();
    
    // reset time cache
    last_time_str[0] = '\0';
    // draw time
    draw_time();
}

/**
 * draws dock
 */
void draw_dock() {
    // just the usual
    
    gui_clearmouse();
    gui_init();
    
    last_time_str[0] = '\0';
    dock_draw_window();
    draw_time();
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
    terminal_active = false;
    if (!dialog_active) {
        
        draw_cursor();
    }
}

/**
 * Handle mouse clicks on dock apps
 */
static bool dock_handle_mouse_click(int mouse_x, int mouse_y) {
    if (!dock_is_active()) {
        return false;
    }
    
    int start_x = FILE_AREA_X + 20;
    int start_y = FILE_AREA_Y;
    
    int app_positions[4][4];
    
    // file explorer
    app_positions[0][0] = start_x - 35;  // x (centered around icon)
    app_positions[0][1] = start_y - 2;   // y
    app_positions[0][2] = 70;            // width
    app_positions[0][3] = 32;            // height (icon + text)
    
    // text editor
    app_positions[1][0] = start_x + FILE_SPACING_X - 35;
    app_positions[1][1] = start_y - 2;
    app_positions[1][2] = 70;
    app_positions[1][3] = 32;

    // terminal
    app_positions[2][0] = start_x + FILE_SPACING_X * 2 - 35;
    app_positions[2][1] = start_y - 2;
    app_positions[2][2] = 70;
    app_positions[2][3] = 32;
    // check for clicks
    for (int i = 0; i < total_apps; i++) {
        int x = app_positions[i][0];
        int y = app_positions[i][1];
        int w = app_positions[i][2];
        int h = app_positions[i][3];
        
        if (mouse_x >= x && mouse_x < x + w && 
            mouse_y >= y && mouse_y < y + h) {
            int previous_selection = selected_app;
            selected_app = i;
            
            if (previous_selection != selected_app) {
                draw_dock();
                draw_time_forced(); 
            }
            
            launch_selected_app();
            return true;
        }
    }
    
    return false;
}

/**
 * launch selected app
 */
static void launch_selected_app() {
    
    gui_clearmouse();
    
    switch (selected_app) {
        case 0:
            explorer_active = true;
            editor_active = false;
            draw_filesplorer();
            break;
            
        case 1:
            dialog_active = true;
            dialog_type = DIALOG_TYPE_NEW_FILE;
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            draw_dialog("New File", "Enter filename:");
            break;
            
        case 2:
            terminal_active = true;
            explorer_active = false;
            editor_active = false;
            dialog_active = false;
            gui_open_terminal();
            break;
            
        default:
            break;
    }
}

/**
 * handle key
 */
bool dock_handle_key(unsigned char key, char scancode) {
    if (dialog_active && dialog_type == DIALOG_TYPE_NEW_FILE) {
        if (scancode == ENTER_KEY_CODE) {
            dialog_active = false;
            dock_mkopen_file();
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            return true;
        } 
        else if (scancode == ESC_KEY_CODE) {
            dialog_active = false;
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            draw_dock();
            return true;
        }
        else if (scancode == BS_KEY_CODE) {
            if (dialog_input_pos > 0) {
                dialog_input_pos--;
                
                int input_len = 0;
                while (input_len < 128 && dialog_input[input_len] != '\0') {
                    input_len++;
                }
                
                for (int i = dialog_input_pos; i < input_len; i++) {
                    dialog_input[i] = dialog_input[i + 1];
                }
                
                draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        else if (scancode == ARROW_LEFT_KEY) {
            // left arrow - move cursor left
            if (dialog_input_pos > 0) {
                dialog_input_pos--;
                draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        else if (scancode == ARROW_RIGHT_KEY) {
            // right arrow - move cursor right
            int input_len = 0;
            while (input_len < 128 && dialog_input[input_len] != '\0') {
                input_len++;
            }
            if (dialog_input_pos < input_len) {
                dialog_input_pos++;
                draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        else if (key >= 32 && key < 127) {
            // printable - insert character instead of overwriting
            int input_len = 0;
            while (input_len < 128 && dialog_input[input_len] != '\0') {
                input_len++;
            }
            
            if (input_len < 128) {
                for (int i = input_len; i > dialog_input_pos; i--) {
                    dialog_input[i] = dialog_input[i - 1];
                }
                
                dialog_input[dialog_input_pos] = key;
                dialog_input_pos++;
                dialog_input[input_len + 1] = '\0';
                draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        return true;
    }
    
    // if we have a dialog and in dock, we handle it
    if (explorer_active || editor_active || terminal_active) {
        return false;
    }
    // change selected app to prev
    int previous_selection = selected_app;
    
    switch (scancode) {
        case ARROW_LEFT_KEY:
            // left
            if (selected_app > 0) {
                selected_app--;
            } else {
                selected_app = total_apps - 1;  
            }
            break;
            
        case ARROW_RIGHT_KEY:
            // right
            if (selected_app < total_apps - 1) {
                selected_app++;
            } else {
                selected_app = 0; 
            }
            break;
            
        case ARROW_UP_KEY:
            // up
            if (selected_app >= FILES_PER_ROW) {
                selected_app -= FILES_PER_ROW;
            }
            break;
            
        case ARROW_DOWN_KEY:
            // down
            if (selected_app + FILES_PER_ROW < total_apps) {
                selected_app += FILES_PER_ROW;
            }
            break;
            
        case ENTER_KEY_CODE:
            // launch application
            launch_selected_app();
            return true;
            
        default:
            return false;
    }
    
    if (previous_selection != selected_app) {
        draw_dock();  // redraw
        draw_time_forced();
    }
    
    return true;
}

/**
 * init
 */
void dock_init() {
    selected_app = 0;
    draw_dock();
}

/**
 * dock active?
 */
bool dock_is_active() {
    return (!explorer_active && !editor_active && !dialog_active && !terminal_active); // crazy logic right here *claps*
}

/**
 * go back to dock
 */
void dock_return() {
    
    gui_clearmouse();
    
    explorer_active = false;
    editor_active = false;
    terminal_active = false;
    dialog_active = false;
    selected_app = 0;
    draw_dock();
}

/**
 * set app selection
 */
void dock_set_selection(int app_index) {
    if (app_index >= 0 && app_index < total_apps) {
        selected_app = app_index;
        if (dock_is_active()) {
            draw_dock();
        }
    }
}

/**
 * create and open a file
 */
void dock_mkopen_file() {
    if (dialog_input[0] != '\0') {
        File* original_cwd = cwd;
        cwd = root;  // go 2 root
        
        // create new file
        filesys_mkfile(dialog_input, "");
        
        // restor original dir
        cwd = original_cwd;
        
        // launch teditor (ted joke)
        editor_active = true;
        explorer_active = false;
        editor_open(dialog_input);
    } else {
        // no filename, go back!!
        draw_dock();
    }
}

/**
 * Handle mouse input for dock
 */
bool dock_handle_mouse() {
    static bool last_left_state = false;
    
    if (!dock_is_active()) {
        return false;
    }
    
    mouse_state_t* mouse = get_mouse_state();
    if (!mouse) {
        return false;
    }
    
    int mouse_x = (mouse->x_position * SCREEN_WIDTH) / 640;
    int mouse_y = (mouse->y_position * SCREEN_HEIGHT) / 480;
    
    if (mouse->left_button) {
        if (!last_left_state) {
            // click detected
            last_left_state = true;
            return dock_handle_mouse_click(mouse_x, mouse_y);
        }
    } else {
        // reset state when released
        last_left_state = false;
    }
    
    return false;
}

/**
 * update time
 */
void dock_update_time() {
    static uint32_t last_time_update = 0;
    static uint32_t last_mouse_recovery = 0;
    
    #define TIME_UPDATE_FREQUENCY 2000
    
    if (dock_is_active() && (ticks - last_time_update >= TIME_UPDATE_FREQUENCY)) {
        draw_time();
        last_time_update = ticks;
    }
    
    if (ticks - last_mouse_recovery >= 500) { 
        if (!dialog_active) {
            extern void gui_updatemouse(void);
            gui_updatemouse();
        }
        last_mouse_recovery = ticks;
    }
}


