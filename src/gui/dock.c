#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "images.h"
//#include "fontdef.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h"
#include "../lib/lib.h"
#include "terminal.h"
#include "../time/rtc.h"

// =============================================================================
// DOCK CONSTANTS AND CONFIGURATION (File Explorer Style)
// =============================================================================
extern void gui_open_text_editor(const char* filename);
extern void gui_draw_filesplorer();

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// File Explorer style window
#define WINDOW_WIDTH 300     // Increased from 280
#define WINDOW_HEIGHT 180    // Increased from 160
#define WINDOW_X ((SCREEN_WIDTH - WINDOW_WIDTH) / 2)
#define WINDOW_Y ((SCREEN_HEIGHT - WINDOW_HEIGHT) / 2)

// Title bar
#define TITLE_BAR_HEIGHT 20

// File area (inside the window)
#define FILE_AREA_X (WINDOW_X + 8)
#define FILE_AREA_Y (WINDOW_Y + TITLE_BAR_HEIGHT + 8)
#define FILE_AREA_WIDTH (WINDOW_WIDTH - 16)
#define FILE_AREA_HEIGHT (WINDOW_HEIGHT - TITLE_BAR_HEIGHT - 16)

// File display (like file explorer)
#define ICON_SIZE 20         // Same as file explorer
#define FILE_SPACING_X 80    // Horizontal spacing between files
#define FILE_SPACING_Y 60    // Vertical spacing between files
#define FILES_PER_ROW 3      // How many files per row

// File Explorer colors
#define WINDOW_BACKGROUND VGA_COLOR_LIGHT_GREY
#define WINDOW_BORDER_OUTER VGA_COLOR_BLACK
#define WINDOW_BORDER_INNER VGA_COLOR_WHITE
#define TITLE_BAR_COLOR VGA_COLOR_BLUE
#define TITLE_TEXT_COLOR VGA_COLOR_WHITE
#define FILE_TEXT_COLOR VGA_COLOR_BLACK
#define SELECTION_COLOR VGA_COLOR_BLUE
#define SELECTION_TEXT_COLOR VGA_COLOR_WHITE

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// Current selection state
static int selected_app = 0;  // 0 = File Explorer, 1 = Text Editor
static const int total_apps = 3;  // Changed from 2 to 3
static uint32_t last_time_update = 0;  // Track last time update
static char last_time_str[32] = "";    // Cache last time string (full format)

// External state variables
extern bool dialog_active;
extern char dialog_input[129];
extern int dialog_input_pos;
extern int dialog_type;
extern void gui_draw_dialog(const char* title, const char* prompt);
extern bool explorer_active;
extern bool editor_active;
extern bool terminal_active;
extern uint32_t ticks;

// Add these missing extern declarations for filesystem
extern FileSystemNode* root;
extern FileSystemNode* cwd;
extern int filesys_mkfile(const char* name, const char* content);

// Add function declarations
static void launch_selected_app(void);
static void handle_shutdown(void);
static void dock_create_and_open_file(void);  // Add this declaration

// Add a new dialog type for file naming
#define DIALOG_TYPE_NEW_FILE 2

// =============================================================================
// FILE EXPLORER STYLE DRAWING FUNCTIONS
// =============================================================================

/**
 * Draws file explorer style window border (matching your explorer exactly)
 */
static void draw_window_border() {
    // Use the same window drawing function as your file explorer
    gui_draw_window_box(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, 
                       VGA_COLOR_BLACK,
                       VGA_COLOR_WHITE,
                       VGA_COLOR_LIGHT_GREY);
    
    // Add the same title bar as your file explorer
    gui_draw_title_bar(WINDOW_X, WINDOW_Y, WINDOW_WIDTH, 15, VGA_COLOR_BLUE);
    
    // Draw title text (same style as explorer)
    gui_draw_text(WINDOW_X + 5, WINDOW_Y + 3, "Welcome to MooseOS", VGA_COLOR_WHITE);
}

/**
 * Draws an icon using bitmap from images.h (file explorer size)
 */
static void draw_app_icon_bitmap(int x, int y, uint8_t bg_color, int app_index) {
    const uint8_t (*icon_bitmap)[16];
    
    // Select appropriate icon based on app type
    switch (app_index) {
        case 0: // File Explorer
            icon_bitmap = folder_icon;
            break;
        case 2: // Terminal
            icon_bitmap = terminal_icon;
            break;
        case 1: // Text Editor
        default:
            icon_bitmap = file_icon;
            break;
    }
    
    for (int row = 0; row < 16; row++) {
        for (int col = 0; col < 16; col++) {
            uint8_t pixel = icon_bitmap[row][col];
            uint8_t color;
            
            // Map bitmap colors to VGA colors
            if (pixel == 0) {
                color = bg_color;  // Transparent/background
            } else {
                color = icon_color_map[pixel];  // Use color mapping from images.h
            }
            
            gui_set_pixel(x + col, y + row, color);
        }
    }
}

/**
 * Draws a single application as a file (exactly like file explorer)
 */
static void draw_app_file(int app_index, const char* filename, int x, int y) {
    bool is_selected = (app_index == selected_app);
    
    // Calculate selection area (icon + text) - properly centered
    int text_width = gui_text_width(filename);
    int selection_width = (text_width > ICON_SIZE) ? text_width : ICON_SIZE;
    int selection_height = ICON_SIZE + 12; // Icon + text height
    
    // Center the selection rectangle on the icon
    int selection_x = x - (selection_width - ICON_SIZE) / 2;
    int selection_y = y;
    
    // Draw selection background exactly like file explorer
    if (is_selected) {
        // Blue selection rectangle - properly centered
        gui_draw_rect(selection_x - 2, selection_y - 2, selection_width + 4, selection_height + 4, SELECTION_COLOR);
        
        // Selection border (like file explorer) - properly centered
        // gui_draw_rect_outline(selection_x - 2, selection_y - 2, selection_width + 4, selection_height + 4, WINDOW_BORDER_OUTER);
        // gui_draw_rect_outline(selection_x - 1, selection_y - 1, selection_width + 2, selection_height + 2, WINDOW_BORDER_INNER);
    }
    
    // Draw icon with appropriate background
    uint8_t icon_bg = is_selected ? SELECTION_COLOR : WINDOW_BACKGROUND;
    
    // Draw appropriate icon based on app index
    draw_app_icon_bitmap(x, y, icon_bg, app_index);
    
    // Draw filename below icon (properly centered on icon)
    int text_x = x + (ICON_SIZE / 2) - (text_width / 2);
    int text_y = y + ICON_SIZE + 2;
    
    uint8_t text_color = is_selected ? SELECTION_TEXT_COLOR : FILE_TEXT_COLOR;
    gui_draw_text(text_x, text_y, filename, text_color);
}

/**
 * Draws all applications as files in file explorer layout
 */
static void draw_application_files() {
    // Calculate starting position
    int start_x = FILE_AREA_X + 20;
    int start_y = FILE_AREA_Y;
    
    // Draw File Explorer as folder
    draw_app_file(0, "File Explorer", start_x, start_y);
    
    // Draw Text Editor as file
    draw_app_file(1, "Text Editor", start_x + FILE_SPACING_X, start_y);
    
    // Draw Terminal as file (below the first row)
    draw_app_file(2, "Terminal", start_x + (FILE_SPACING_X * 2), start_y);
}

/**
 * Draw a small time display in the bottom left of the dock
 */
static void draw_dock_time_box() {
    // Get current time with timezone offset applied
    rtc_time_t current_time = rtc_get_local_time();
    
    // Create time string manually (HH:MM:SS DD/MM/YYYY UTC±X format - same as terminal)
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
    
    // Add date (DD/MM/YYYY format)
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
    
    // Add timezone info
    int offset = rtc_get_timezone_offset();
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
    
    // Check if time has changed (compare full string)
    bool time_changed = false;
    for (int i = 0; time_str[i] != '\0' && i < 31; i++) {
        if (last_time_str[i] != time_str[i]) {
            time_changed = true;
            break;
        }
    }
    
    // Only redraw if time has actually changed
    if (time_changed) {
        // Status bar dimensions (bottom of dock, same style as terminal)
        int status_bar_width = WINDOW_WIDTH - 16;
        int status_bar_height = 15;
        int status_bar_x = WINDOW_X + 8;
        int status_bar_y = WINDOW_Y + WINDOW_HEIGHT - status_bar_height - 8;
        
        // Draw status bar background (same style as terminal)
        gui_draw_rect(status_bar_x, status_bar_y, status_bar_width, status_bar_height, VGA_COLOR_LIGHT_GREY);
        gui_draw_hline(status_bar_x, status_bar_x + status_bar_width - 1, status_bar_y, VGA_COLOR_DARK_GREY);
        
        // Draw time text (left-aligned like terminal)
        gui_draw_text(status_bar_x + 5, status_bar_y + 3, time_str, VGA_COLOR_BLACK);
        
        // Cache the current string
        for (int i = 0; i < 32; i++) {
            last_time_str[i] = time_str[i];
            if (time_str[i] == '\0') break;
        }
    }
}

/**
 * Main dock drawing function (exactly like file explorer)
 */
static void draw_dock_window() {
    // Clear screen with the same background as file explorer
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // Draw window using the same
    draw_window_border();
    
    // Draw files inside the window
    draw_application_files();
    
    // Reset time cache since we're doing a full redraw
    last_time_str[0] = '\0';
    
    // Draw small time display in bottom left
    draw_dock_time_box();
}

// =============================================================================
// MAIN DOCK INTERFACE
// =============================================================================

/**
 * Main function to draw the complete file explorer style dock
 */
void gui_draw_dock() {
    // Initialize VGA if needed
    gui_init();
    
    // Reset time cache for full redraw
    last_time_str[0] = '\0';
    
    // Draw the complete interface
    draw_dock_window();
    
    // Always update time display
    draw_dock_time_box();
    
    // Set dock as active
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
}

/**
 * Launches the selected application
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
            
        default:
            break;
    }
}

/**
 * Handles shutdown sequence
 */
static void handle_shutdown() {
    // Clear screen
    gui_clear_screen(VGA_COLOR_BLACK);
    
    // Draw shutdown message
    const char* shutdown_msg = "MooseOS is shutting down...";
    int msg_width = gui_text_width(shutdown_msg);
    int msg_x = (SCREEN_WIDTH - msg_width) / 2;
    int msg_y = (SCREEN_HEIGHT / 2) - 4;
    
    gui_draw_text(msg_x, msg_y, shutdown_msg, VGA_COLOR_WHITE);
    
    // Simple delay loop
    for (volatile int i = 0; i < 1000000; i++) {
        // Wait
    }
    
    // Return to dock
    gui_draw_dock();
}

// =============================================================================
// INPUT HANDLING
// =============================================================================

/**
 * Handles keyboard input for the file explorer style dock
 */
bool gui_handle_dock_key(unsigned char key, char scancode) {
    // Handle dialog input when dialog is active
    if (dialog_active && dialog_type == DIALOG_TYPE_NEW_FILE) {
        // Handle dialog keys directly here
        if (scancode == ENTER_KEY_CODE) {
            // User pressed Enter - create file and open editor
            dialog_active = false;
            dock_create_and_open_file();
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            return true;
        } 
        else if (scancode == ESC_KEY_CODE) {
            // User pressed Escape - cancel and return to dock
            dialog_active = false;
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            gui_draw_dock();
            return true;
        }
        else if (scancode == BS_KEY_CODE) {
            // Handle backspace
            if (dialog_input_pos > 0) {
                dialog_input_pos--;
                dialog_input[dialog_input_pos] = '\0';
                gui_draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        else if (key >= 32 && key < 127) {
            // Handle printable characters
            if (dialog_input_pos < 128) {
                dialog_input[dialog_input_pos] = key;
                dialog_input_pos++;
                dialog_input[dialog_input_pos] = '\0';
                gui_draw_dialog("New File", "Enter filename:");
            }
            return true;
        }
        return true; // Consume all keys when dialog is active
    }
    
    // Only handle dock navigation if we're the active interface and no dialog
    if (explorer_active || editor_active || terminal_active) {
        return false;
    }
    
    int previous_selection = selected_app;
    
    switch (scancode) {
        case ARROW_LEFT_KEY:
            // Move selection left
            if (selected_app > 0) {
                selected_app--;
            } else {
                selected_app = total_apps - 1;  // Wrap around
            }
            break;
            
        case ARROW_RIGHT_KEY:
            // Move selection right
            if (selected_app < total_apps - 1) {
                selected_app++;
            } else {
                selected_app = 0;  // Wrap around
            }
            break;
            
        case ARROW_UP_KEY:
            // Move selection up
            if (selected_app >= FILES_PER_ROW) {
                selected_app -= FILES_PER_ROW;
            }
            break;
            
        case ARROW_DOWN_KEY:
            // Move selection down
            if (selected_app + FILES_PER_ROW < total_apps) {
                selected_app += FILES_PER_ROW;
            }
            break;
            
        case ENTER_KEY_CODE:
            // Launch selected application
            launch_selected_app();
            return true;
            
        default:
            // Any other key redraws dock
            if (key != 0) {
                gui_draw_dock();
                return true;
            }
            return false;
    }
    
    // Always redraw to update time display
    if (previous_selection != selected_app) {
        gui_draw_dock();  // Full redraw if selection changed
    } else {
        // Just update time display if no selection change
        draw_dock_time_box();
    }
    
    return true;
}

// =============================================================================
// INITIALIZATION AND UTILITY FUNCTIONS
// =============================================================================

/**
 * Initialize the dock system
 */
void dock_init() {
    selected_app = 0;
    gui_draw_dock();
}

/**
 * Check if dock is currently active
 */
bool dock_is_active() {
    return (!explorer_active && !editor_active && !dialog_active && !terminal_active);
}

/**
 * Return to dock from other applications
 */
void dock_return() {
    explorer_active = false;
    editor_active = false;
    dialog_active = false;
    selected_app = 0;
    gui_draw_dock();
}

/**
 * Get current application selection
 */
int dock_get_selection() {
    return selected_app;
}

/**
 * Set application selection programmatically
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
 * Creates and opens a new file with the name from dialog input
 */
void dock_create_and_open_file() {
    if (dialog_input[0] != '\0') {
        // Make sure we're at root directory
        FileSystemNode* original_cwd = cwd;
        cwd = root;  // Temporarily switch to root
        
        // Create the new file (empty content)
        filesys_mkfile(dialog_input, "");
        
        // Restore original directory
        cwd = original_cwd;
        
        // Launch text editor with the new file
        editor_active = true;
        explorer_active = false;
        gui_open_text_editor(dialog_input);
    } else {
        // No filename entered, just return to dock
        gui_draw_dock();
    }
}

/**
 * Update dock time display (call this from main loop for constant updates)
 */
void dock_update_time() {
    // Only update if dock is active and visible
    if (dock_is_active() && terminal_active == false && editor_active == false && dialog_active == false) {
        draw_dock_time_box();
    }
}

/**
 * Force redraw of time box (call when dock is redrawn)
 */
static void force_time_box_redraw() {
    last_time_str[0] = '\0';
    draw_dock_time_box();
}