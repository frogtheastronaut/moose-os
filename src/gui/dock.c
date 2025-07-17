/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

// lots and lots of includes
#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "../kernel/include/mouse.h"
#include "include/images.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h"
#include "../lib/lib.h"
#include "include/terminal.h"
#include "include/pong.h"
#include "../time/rtc.h"

// defines

// externs
extern void editor_open(const char* filename);
extern void gui_draw_filesplorer();

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
static int selected_app = 0;  // 0 = File Explorer, 1 = Text Editor, 2 = Terminal, 3 = Pong
static const int total_apps = 4;  
static uint32_t last_time_update = 0;  // last update
static char last_time_str[32] = "";    // last time as in time time

// extern variables
extern bool dialog_active;
extern char dialog_input[129];
extern int dialog_input_pos;
extern int dialog_type;
extern void gui_draw_dialog(const char* title, const char* prompt);
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern bool pong_active;
extern uint32_t ticks;

// more
extern File* root;
extern File* cwd;
extern int filesys_mkfile(const char* name, const char* content);

// func decs
static void launch_selected_app(void);
static void handle_shutdown(void);
static void dock_mkopen_file(void);  

// huh
#define DIALOG_TYPE_NEW_FILE 2

/**
 * draw file explorer
 */
static void draw_window_border() {
    gui_draw_window_box(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, 
                       VGA_COLOR_BLACK,
                       VGA_COLOR_WHITE,
                       VGA_COLOR_LIGHT_GREY);
    
    gui_draw_title_bar(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, 15, VGA_COLOR_BLUE);
    gui_draw_text(WINDOW_X + 5, WINDOW_Y + 3, "Welcome to MooseOS", VGA_COLOR_WHITE);
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
        case 3: // pongongong
            icon_bitmap = pong_icon;
            break;
        case 1: // ted
        default:
            icon_bitmap = file_icon;
            break;
    }
    
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint8_t pixel = icon_bitmap[row][col];
            uint8_t color;
            
            // color = colour
            if (pixel == 0) {
                color = bg_color;  // transparent
            } else {
                color = icon_color_map[pixel];  // color mapping from images.h
            }
            // sets pixel
            gui_set_pixel(x + col, y + row, color);
        }
    }
}

/**
 * draw app
 */
static void dock_draw_app(int app_index, const char* filename, int x, int y) {
    bool is_selected = (app_index == selected_app);
    
    // selection area
    int text_width = gui_text_width(filename);
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
        gui_draw_rect(selection_x - 2, selection_y - 2, selection_width + 4, selection_height + 4, SELECTION_COLOR);
    }
    
    // icon has correct bg
    uint8_t icon_bg = is_selected ? SELECTION_COLOR : WINDOW_BACKGROUND;
    
    // draw icon
    dock_draw_icon(x, y, icon_bg, app_index);
    
    // draw filename
    int text_x = x + (ICON_SIZE / 2) - (text_width / 2);
    int text_y = y + ICON_SIZE + 2;
    
    uint8_t text_color = is_selected ? SELECTION_TEXT_COLOR : FILE_TEXT_COLOR;
    gui_draw_text(text_x, text_y, filename, text_color);
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
    
    // pong
    dock_draw_app(3, "Pong", start_x, start_y + FILE_SPACING_Y);
}

/**
 * time displaydsdf
 */
static void dock_draw_time() {
    // get current time
    rtc_time current_time = rtc_get_time();
    
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
    
    // timezone
    int offset = timezone_offset; // from rtc.c
    int ltime_str = 0;
    time_str[19] = ' ';
    time_str[20] = 'U';
    time_str[21] = 'T';
    time_str[22] = 'C';
    if (offset >= 0) {
        time_str[23] = '+';
        if (offset < 10) {
            time_str[24] = '0' + offset;
            time_str[25] = '\0';
            ltime_str = 25;
        } else {
            time_str[24] = '0' + (offset / 10);
            time_str[25] = '0' + (offset % 10);
            time_str[26] = '\0';
            ltime_str = 26;
        }
    } else {
        int abs_offset = -offset;
        time_str[23] = '-';
        if (abs_offset < 10) {
            time_str[24] = '0' + abs_offset;
            time_str[25] = '\0';
            ltime_str = 25;
        } else {
            time_str[24] = '0' + (abs_offset / 10);
            time_str[25] = '0' + (abs_offset % 10);
            time_str[26] = '\0';
            ltime_str = 26;
        }
    }
    if (offset >= -12 && offset <= 14) {
        // nuthin'
    } else {
        time_str[ltime_str] = '?';
        time_str[ltime_str + 1] = '\0';
    }
    
    // check if time changed
    bool time_changed = false;
    for (int i = 0; time_str[i] != '\0' && i < 31; i++) {
        if (last_time_str[i] != time_str[i]) {
            time_changed = true;
            break;
        }
    }
    
    // redraw if time changed
    if (time_changed) {
        // status bar
        int status_bar_width = WINDOW_WIDTH - 16;
        int status_bar_height = 15;
        int status_bar_x = WINDOW_X + 8;
        int status_bar_y = WINDOW_Y + WINDOW_HEIGHT - status_bar_height - 8;
        
        // bar background
        gui_draw_rect(status_bar_x, status_bar_y, status_bar_width, status_bar_height, VGA_COLOR_LIGHT_GREY);
        gui_draw_hline(status_bar_x, status_bar_x + status_bar_width - 1, status_bar_y, VGA_COLOR_DARK_GREY);
        
        // text
        gui_draw_text(status_bar_x + 5, status_bar_y + 3, time_str, VGA_COLOR_BLACK);
        
        // c4ch3 
        for (int i = 0; i < 32; i++) {
            last_time_str[i] = time_str[i];
            if (time_str[i] == '\0') break;
        }
    }
}

/**
 *  draw dock
 */
static void dock_draw_window() {
    // clear screen
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // draw window
    draw_window_border();
    
    // draw files
    dock_draw_apps();
    
    // reset time cache
    last_time_str[0] = '\0';
    
    // draw time
    dock_draw_time();
}

/**
 * Main function to draw the complete file explorer style dock
 */
void gui_draw_dock() {
    // Initialize VGA if needed
    gui_init();
    
    // Reset time cache for full redraw
    last_time_str[0] = '\0';
    
    // Draw the complete interface
    dock_draw_window();
    
    // Always update time display
    dock_draw_time();
    
    // Set dock as active
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
}

/**
 * launch selected app
 */
static void launch_selected_app() {
    switch (selected_app) {
        case 0:
            // Launch File Explorer
            explorer_active = true;
            editor_active = false;
            gui_draw_filesplorer();
            break;
            
        case 1:
            // Launch Text Editor - show dialog to get filename
            dialog_active = true;
            dialog_type = DIALOG_TYPE_NEW_FILE;
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            gui_draw_dialog("New File", "Enter filename:");
            break;
            
        case 2:
            // Launch Terminal
            terminal_active = true;
            explorer_active = false;
            editor_active = false;
            dialog_active = false;
            gui_open_terminal();
            break;
            
        case 3:
            // Launch Pong
            pong_active = true;
            explorer_active = false;
            editor_active = false;
            terminal_active = false;
            dialog_active = false;
            pong_init_game();
            gui_draw_pong();
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
            // user pressed Enter - create file and open editor
            dialog_active = false;
            dock_mkopen_file();
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            return true;
        } 
        else if (scancode == ESC_KEY_CODE) {
            // user pressed Esc - cancel and return to dock
            dialog_active = false;
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            gui_draw_dock();
            return true;
        }
        else if (scancode == BS_KEY_CODE) {
            // backspace
            if (dialog_input_pos > 0) {
                dialog_input_pos--;
                dialog_input[dialog_input_pos] = '\0';
                gui_draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        else if (key >= 32 && key < 127) {
            // printable
            if (dialog_input_pos < 128) {
                dialog_input[dialog_input_pos] = key;
                dialog_input_pos++;
                dialog_input[dialog_input_pos] = '\0';
                gui_draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        return true;
    }
    
    // if we have a dialog and in dock, we handle it
    if (explorer_active || editor_active || terminal_active || pong_active) {
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
            // Launch selected application
            launch_selected_app();
            return true;
            
        default:
            // just redraw it alr
            if (key != 0) {
                gui_draw_dock();
                return true;
            }
            return false;
    }
    
    // redraw time display
    if (previous_selection != selected_app) {
        gui_draw_dock();  // Full redraw if selection changed
     }
    
    return true;
}

/**
 * init
 */
void dock_init() {
    selected_app = 0;
    gui_draw_dock();
}

/**
 * dock active?
 */
bool dock_is_active() {
    return (!explorer_active && !editor_active && !dialog_active && !terminal_active && !pong_active); // crazy logic right here *claps*
}

/**
 * go back to dock
 */
void dock_return() {
    explorer_active = false;
    editor_active = false;
    terminal_active = false;
    pong_active = false;
    dialog_active = false;
    selected_app = 0;
    gui_draw_dock();
}

/**
 * set app selection
 */
void dock_set_selection(int app_index) {
    if (app_index >= 0 && app_index < total_apps) {
        selected_app = app_index;
        if (dock_is_active()) {
            gui_draw_dock();
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
        gui_draw_dock();
    }
}

/**
 * update time
 */
void dock_update_time() {
    // check if dock is visible
    if (dock_is_active() && terminal_active == false && editor_active == false && dialog_active == false && pong_active == false) {
        dock_draw_time();
        dock_draw_mouse_info();
    }
    
    // Always update mouse cursor regardless of what's active
    gui_update_mouse_cursor();
}

/**
 * display mouse information
 */
void dock_draw_mouse_info() {
    mouse_state_t* mouse = get_mouse_state();
    
    // Create mouse info string
    char mouse_str[64];
    mouse_str[0] = 'M';
    mouse_str[1] = ':';
    mouse_str[2] = ' ';
    
    // X position
    int x_pos = mouse->x_position;
    int str_pos = 3;
    if (x_pos >= 100) {
        mouse_str[str_pos++] = '0' + (x_pos / 100);
        x_pos %= 100;
    }
    if (x_pos >= 10 || mouse->x_position >= 100) {
        mouse_str[str_pos++] = '0' + (x_pos / 10);
        x_pos %= 10;
    }
    mouse_str[str_pos++] = '0' + x_pos;
    
    mouse_str[str_pos++] = ',';
    
    // Y position
    int y_pos = mouse->y_position;
    if (y_pos >= 100) {
        mouse_str[str_pos++] = '0' + (y_pos / 100);
        y_pos %= 100;
    }
    if (y_pos >= 10 || mouse->y_position >= 100) {
        mouse_str[str_pos++] = '0' + (y_pos / 10);
        y_pos %= 10;
    }
    mouse_str[str_pos++] = '0' + y_pos;
    
    // Buttons
    mouse_str[str_pos++] = ' ';
    mouse_str[str_pos++] = '[';
    mouse_str[str_pos++] = mouse->left_button ? 'L' : '-';
    mouse_str[str_pos++] = mouse->middle_button ? 'M' : '-';
    mouse_str[str_pos++] = mouse->right_button ? 'R' : '-';
    mouse_str[str_pos++] = ']';
    mouse_str[str_pos] = '\0';
    
    // Draw mouse info on the right side of status bar
    int status_bar_x = WINDOW_X + 8;
    int status_bar_y = WINDOW_Y + WINDOW_HEIGHT - 15 - 8;
    int text_x = status_bar_x + WINDOW_WIDTH - 16 - gui_text_width(mouse_str) - 5;
    
    // Clear area first
    gui_draw_rect(text_x - 5, status_bar_y, gui_text_width(mouse_str) + 10, 15, VGA_COLOR_LIGHT_GREY);
    
    // Draw mouse info
    gui_draw_text(text_x, status_bar_y + 3, mouse_str, VGA_COLOR_BLACK);
}
