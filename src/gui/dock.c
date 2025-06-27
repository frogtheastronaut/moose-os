#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "images.h"
//#include "fontdef.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h"
#include "../lib/lib.h"

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
static const int total_apps = 2;

// External state variables
extern bool dialog_active;
extern char dialog_input[129];
extern int dialog_input_pos;
extern int dialog_type;
extern void gui_draw_dialog(const char* title, const char* prompt);
extern bool explorer_active;
extern bool editor_active;
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
static void draw_app_icon_bitmap(int x, int y, uint8_t bg_color, bool is_folder) {
    const uint8_t (*icon_bitmap)[16] = is_folder ? folder_icon : file_icon;
    
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
    
    // Draw appropriate icon (folder for file explorer, file for text editor)
    bool is_folder = (app_index == 0);
    draw_app_icon_bitmap(x, y, icon_bg, is_folder);
    
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
    // Calculate starting position (centered in file area)
    int start_x = FILE_AREA_X + 20;
    int start_y = FILE_AREA_Y   ;
    
    // Draw File Explorer as folder
    draw_app_file(0, "File Explorer", start_x, start_y);
    
    // Draw Text Editor as file (to the right)
    draw_app_file(1, "Text Editor", start_x + FILE_SPACING_X, start_y);
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
    
    // Draw the complete interface
    draw_dock_window();
    
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
    if (explorer_active || editor_active) {
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
        case ARROW_DOWN_KEY:
            // For future expansion with multiple rows
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
    
    // Redraw if selection changed
    if (previous_selection != selected_app) {
        gui_draw_dock();
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
    return (!explorer_active && !editor_active && !dialog_active);
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