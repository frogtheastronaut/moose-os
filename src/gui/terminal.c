#include "gui.h"
#include "../kernel/include/tty.h"
#include "../kernel/include/keyboard.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../lib/lib.h"
#include "../time/rtc.h"

// Add declarations for your custom functions from lib.c
extern void copyStr(char* dest, const char* src);
extern int strEqual(const char* a, const char* b);
extern int msnprintf(char *buffer, int size, const char *format, ...);
extern size_t strlen(const char* str);
extern char* strcat(char* dest, const char* src);
extern const char* strip_whitespace(const char* str);
extern void dock_return(void);  // Add this line
// =============================================================================
// TERMINAL CONSTANTS
// =============================================================================

#define TERMINAL_WIDTH 300
#define TERMINAL_HEIGHT 180
#define TERMINAL_X ((SCREEN_WIDTH - TERMINAL_WIDTH) / 2)
#define TERMINAL_Y ((SCREEN_HEIGHT - TERMINAL_HEIGHT) / 2)

// Terminal display area
#define TERM_AREA_X (TERMINAL_X + 8)
#define TERM_AREA_Y (TERMINAL_Y + 28)
#define TERM_AREA_WIDTH (TERMINAL_WIDTH - 16)
#define TERM_AREA_HEIGHT (TERMINAL_HEIGHT - 36)

// Terminal colors
#define TERM_BG_COLOR VGA_COLOR_BLACK
#define TERM_TEXT_COLOR VGA_COLOR_WHITE
#define TERM_PROMPT_COLOR VGA_COLOR_GREEN
#define TERM_ERROR_COLOR VGA_COLOR_RED

// Terminal settings
#define MAX_COMMAND_LEN 64
#define MAX_LINES 15
#define CHARS_PER_LINE 35

// =============================================================================
// TERMINAL STATE
// =============================================================================

static char command_buffer[MAX_COMMAND_LEN + 1] = "";
static int command_pos = 0;
static char terminal_lines[MAX_LINES][CHARS_PER_LINE + 1];
static int current_line = 0;
static int scroll_offset = 0;
static bool terminal_active = false;

// External variables
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern FileSystemNode* root;
extern FileSystemNode* cwd;

// =============================================================================
// TERMINAL FUNCTIONS
// =============================================================================

/**
 * Initialize terminal state
 */
void terminal_init() {
    // Clear all lines
    for (int i = 0; i < MAX_LINES; i++) {
        terminal_lines[i][0] = '\0';
    }
    current_line = 0;
    scroll_offset = 0;
    command_buffer[0] = '\0';
    command_pos = 0;
}

/**
 * Add a line to terminal output
 */
static void terminal_add_line(const char* text, uint8_t color) {
    if (current_line >= MAX_LINES) {
        // Scroll up - move all lines up by one
        for (int i = 0; i < MAX_LINES - 1; i++) {
            // Copy each character manually to ensure proper copying
            int j = 0;
            while (j < CHARS_PER_LINE && terminal_lines[i + 1][j] != '\0') {
                terminal_lines[i][j] = terminal_lines[i + 1][j];
                j++;
            }
            terminal_lines[i][j] = '\0';
        }
        current_line = MAX_LINES - 1;
    }
    
    // Add new line - copy manually to ensure proper copying
    int i = 0;
    while (i < CHARS_PER_LINE && text[i] != '\0') {
        terminal_lines[current_line][i] = text[i];
        i++;
    }
    terminal_lines[current_line][i] = '\0';
    current_line++;
}

/**
 * Print text to terminal
 */
static void terminal_print(const char* text) {
    terminal_add_line(text, TERM_TEXT_COLOR);
}

/**
 * Print error message to terminal
 */
static void terminal_print_error(const char* text) {
    terminal_add_line(text, TERM_ERROR_COLOR);
}

/**
 * Get current directory name
 */
static const char* get_current_dir_name() {
    if (cwd == root) {
        return "/";
    }
    // For non-root directories, we need to add a slash
    static char dir_with_slash[MAX_NAME_LEN + 2];  // +2 for slash and null terminator
    msnprintf(dir_with_slash, sizeof(dir_with_slash), "%s/", cwd->name);
    return dir_with_slash;
}

/**
 * Execute a terminal command
 */
static void execute_command(const char* cmd) {
    // Strip leading and trailing spaces from the command
    cmd = strip_whitespace(cmd);
    
    // Add command to history
    char prompt_line[CHARS_PER_LINE + 1];
    msnprintf(prompt_line, sizeof(prompt_line), "%s# %s", get_current_dir_name(), cmd);  // Use msnprintf instead of snprintf
    terminal_add_line(prompt_line, TERM_PROMPT_COLOR);
    
    // Parse and execute command
    if (strlen(cmd) == 0) {
        return; // Empty command
    }
    
    // Help command - use strEqual instead of strcmp
    if (strEqual(cmd, "help")) {
        terminal_print("Welcome to the MooseOS Terminal");
        terminal_print("help - Show this help");
        terminal_print("ls - List files");
        terminal_print("cd <dir> - Change directory");
        terminal_print("mkdir <name> - Create directory");
        terminal_print("touch <name> - Create file");
        terminal_print("cat <file> - Show file content");
        terminal_print("clear - Clear terminal");
        terminal_print("exit - Exit terminal");
    }
    // List files
    else if (strEqual(cmd, "ls")) {
        if (cwd->folder.childCount == 0) {
            terminal_print("Directory is empty.");
        } else {
            for (int i = 0; i < cwd->folder.childCount; i++) {
                FileSystemNode* child = cwd->folder.children[i];
                char line[CHARS_PER_LINE + 1];
                if (child->type == FOLDER_NODE) {
                    msnprintf(line, sizeof(line), "[DIR]  %s", child->name);
                } else {
                    msnprintf(line, sizeof(line), "[FILE] %s", child->name);
                }
                terminal_print(line);
            }
        }
    }
    // Change directory - create custom string comparison for prefixes
    else if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
        const char* dirname = cmd + 3;
        if (strEqual(dirname, "..")) {
            if (cwd->parent) {
                cwd = cwd->parent;
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Changed to %s", get_current_dir_name());
                terminal_print(line);
            } else {
                terminal_print_error("Already at root");
            }
        } else if (strEqual(dirname, "/")) {
            cwd = root;
            terminal_print("Changed to /");
        } else {
            bool found = false;
            for (int i = 0; i < cwd->folder.childCount; i++) {
                FileSystemNode* child = cwd->folder.children[i];
                if (child->type == FOLDER_NODE && strEqual(child->name, dirname)) {
                    cwd = child;
                    char line[CHARS_PER_LINE + 1];
                    msnprintf(line, sizeof(line), "Changed to %s", dirname);
                    terminal_print(line);
                    found = true;
                    break;
                }
            }
            if (!found) {
                terminal_print_error("Directory not found");
            }
        }
    }
    // Create directory
    else if (cmd[0] == 'm' && cmd[1] == 'k' && cmd[2] == 'd' && cmd[3] == 'i' && cmd[4] == 'r' && cmd[5] == ' ') {
        const char* dirname = cmd + 6;
        if (strlen(dirname) > 0) {
            filesys_mkdir(dirname);
            char line[CHARS_PER_LINE + 1];
            msnprintf(line, sizeof(line), "Created directory %s", dirname);
            terminal_print(line);
        } else {
            terminal_print_error("Usage: mkdir <name>");
        }
    }
    // Create file
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' && cmd[4] == 'h' && cmd[5] == ' ') {
        const char* filename = cmd + 6;
        if (strlen(filename) > 0) {
            filesys_mkfile(filename, "");
            char line[CHARS_PER_LINE + 1];
            msnprintf(line, sizeof(line), "Created file %s", filename);
            terminal_print(line);
        } else {
            terminal_print_error("Usage: touch <name>");
        }
    }
    // Show file content
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
        const char* filename = cmd + 4;
        
        // Search for the file in current directory
        bool found = false;
        for (int i = 0; i < cwd->folder.childCount; i++) {
            FileSystemNode* child = cwd->folder.children[i];
            if (child->type == FILE_NODE && strEqual(child->name, filename)) {
                found = true;
                if (strlen(child->file.content) > 0) {
                    // Split content into multiple lines if needed
                    char* content = child->file.content;
                    char line[CHARS_PER_LINE + 1];
                    int pos = 0;
                    for (int j = 0; content[j] != '\0'; j++) {
                        if (content[j] == '\n' || pos >= CHARS_PER_LINE - 1) {
                            line[pos] = '\0';
                            terminal_print(line);
                            pos = 0;
                            if (content[j] != '\n') {
                                line[pos++] = content[j];
                            }
                        } else {
                            line[pos++] = content[j];
                        }
                    }
                    if (pos > 0) {
                        line[pos] = '\0';
                        terminal_print(line);
                    }
                } else {
                    terminal_print("File is empty");
                }
                break;
            }
        }
        if (!found) {
            terminal_print_error("File not found");
        }
    }
    // Clear terminal
    else if (strEqual(cmd, "clear")) {
        terminal_init();
    }
    // Show current time and date
    else if (strEqual(cmd, "time")) {
        rtc_time_t local_time = rtc_get_local_time();
        char time_line[CHARS_PER_LINE + 1];
        char date_line[CHARS_PER_LINE + 1];
        
        // Format time (HH:MM:SS) - manual formatting
        char hour_str[3], min_str[3], sec_str[3];
        if (local_time.hours < 10) {
            hour_str[0] = '0';
            hour_str[1] = '0' + local_time.hours;
        } else {
            hour_str[0] = '0' + (local_time.hours / 10);
            hour_str[1] = '0' + (local_time.hours % 10);
        }
        hour_str[2] = '\0';
        
        if (local_time.minutes < 10) {
            min_str[0] = '0';
            min_str[1] = '0' + local_time.minutes;
        } else {
            min_str[0] = '0' + (local_time.minutes / 10);
            min_str[1] = '0' + (local_time.minutes % 10);
        }
        min_str[2] = '\0';
        
        if (local_time.seconds < 10) {
            sec_str[0] = '0';
            sec_str[1] = '0' + local_time.seconds;
        } else {
            sec_str[0] = '0' + (local_time.seconds / 10);
            sec_str[1] = '0' + (local_time.seconds % 10);
        }
        sec_str[2] = '\0';
        
        // Format date (DD/MM/YYYY) - manual formatting
        char month_str[3], day_str[3], year_str[5];
        if (local_time.month < 10) {
            month_str[0] = '0';
            month_str[1] = '0' + local_time.month;
        } else {
            month_str[0] = '0' + (local_time.month / 10);
            month_str[1] = '0' + (local_time.month % 10);
        }
        month_str[2] = '\0';
        
        if (local_time.day < 10) {
            day_str[0] = '0';
            day_str[1] = '0' + local_time.day;
        } else {
            day_str[0] = '0' + (local_time.day / 10);
            day_str[1] = '0' + (local_time.day % 10);
        }
        day_str[2] = '\0';
        
        // Format year as full 4-digit year (20XX)
        year_str[0] = '2';
        year_str[1] = '0';
        if (local_time.year < 10) {
            year_str[2] = '0';
            year_str[3] = '0' + local_time.year;
        } else {
            year_str[2] = '0' + (local_time.year / 10);
            year_str[3] = '0' + (local_time.year % 10);
        }
        year_str[4] = '\0';
        
        // Display both time and date
        msnprintf(time_line, sizeof(time_line), "Time: %s:%s:%s", hour_str, min_str, sec_str);
        terminal_print(time_line);
        msnprintf(date_line, sizeof(date_line), "Date: %s/%s/%s", day_str, month_str, year_str);
        terminal_print(date_line);
    }
    // Set timezone offset
    else if (cmd[0] == 's' && cmd[1] == 'e' && cmd[2] == 't' && cmd[3] == 't' && cmd[4] == 'i' && 
             cmd[5] == 'm' && cmd[6] == 'e' && cmd[7] == 'z' && cmd[8] == 'o' && cmd[9] == 'n' && 
             cmd[10] == 'e' && cmd[11] == ' ') {
        const char* offset_str = cmd + 12;
        if (strlen(offset_str) > 0) {
            // Simple integer parsing (supports negative numbers)
            int offset = 0;
            int sign = 1;
            int i = 0;
            
            if (offset_str[0] == '-') {
                sign = -1;
                i = 1;
            } else if (offset_str[0] == '+') {
                i = 1;
            }
            
            // Parse digits
            while (offset_str[i] >= '0' && offset_str[i] <= '9') {
                offset = offset * 10 + (offset_str[i] - '0');
                i++;
            }
            
            offset *= sign;
            
            // Validate range (-12 to +14 covers most timezones)
            if (offset >= -12 && offset <= 14) {
                rtc_set_timezone_offset(offset);
                terminal_print("Timezone updated");
            } else {
                terminal_print_error("Invalid timezone (must be from -12 to +14)");
            }
        } else {
            terminal_print_error("Usage: settimezone <hours>");
        }
    }
    // Unknown command
    else {
        char line[CHARS_PER_LINE + 1];
        msnprintf(line, sizeof(line), "Unknown command: %s", cmd);
        terminal_print_error(line);
        terminal_print("Type 'help' for commands");
    }
}

/**
 * Draw terminal window
 */
static void draw_terminal_window() {
    // Clear screen
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // Draw window border (file explorer style)
    gui_draw_window_box(TERMINAL_X, TERMINAL_Y, TERMINAL_WIDTH, TERMINAL_HEIGHT,
                       VGA_COLOR_BLACK, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY);
    
    // Draw title bar
    gui_draw_title_bar(TERMINAL_X, TERMINAL_Y, TERMINAL_WIDTH, 15, VGA_COLOR_BLUE);
    gui_draw_text(TERMINAL_X + 5, TERMINAL_Y + 3, "MooseOS Terminal", VGA_COLOR_WHITE);
    
    // Draw terminal area background
    gui_draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOR);
    gui_draw_rect_outline(TERM_AREA_X - 1, TERM_AREA_Y - 1, TERM_AREA_WIDTH + 2, TERM_AREA_HEIGHT + 2, VGA_COLOR_DARK_GREY);
}

/**
 * Draw terminal content
 */
static void draw_terminal_content() {
    int y_pos = TERM_AREA_Y + 5;
    int visible_lines = (TERM_AREA_HEIGHT - 35) / 10; // Leave space for status bar (reduced from 20 to 35)
    
    // Draw terminal output lines
    int start_line = (current_line > visible_lines) ? current_line - visible_lines : 0;
    for (int i = start_line; i < current_line && i < start_line + visible_lines; i++) {
        if (terminal_lines[i][0] != '\0') {
            gui_draw_text(TERM_AREA_X + 5, y_pos, terminal_lines[i], TERM_TEXT_COLOR);
            y_pos += 10;
        }
    }
    
    // Draw current command prompt
    char prompt[CHARS_PER_LINE + 1];
    msnprintf(prompt, sizeof(prompt), "%s# %s", get_current_dir_name(), command_buffer);  // Use msnprintf
    gui_draw_text(TERM_AREA_X + 5, y_pos, prompt, TERM_PROMPT_COLOR);
    
    // Draw cursor
    int cursor_x = TERM_AREA_X + 5 + gui_text_width(prompt);
    gui_draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOR);
}



/**
 * Main terminal drawing function
 */
void gui_draw_terminal() {
    gui_init();
    draw_terminal_window();
    draw_terminal_content();
    
    terminal_active = true;
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
}

/**
 * Handle terminal keyboard input
 */
bool gui_handle_terminal_key(unsigned char key, char scancode) {
    if (!terminal_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            // Exit terminal and return to dock
            terminal_active = false;
            dock_return();
            return true;
            
        case ENTER_KEY_CODE:
            // Execute command
            execute_command(command_buffer);
            command_buffer[0] = '\0';
            command_pos = 0;
            gui_draw_terminal(); // Redraw
            return true;
            
        case BS_KEY_CODE:
            // Backspace
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                gui_draw_terminal(); // Redraw
            }
            return true;
            
        default:
            // Handle printable characters
            if (key >= 32 && key < 127 && command_pos < MAX_COMMAND_LEN) {
                command_buffer[command_pos] = key;
                command_pos++;
                command_buffer[command_pos] = '\0';
                gui_draw_terminal(); // Redraw
                return true;  // Only return true if we handled the key
            } 
    }
}

/**
 * Initialize terminal app
 */
void terminal_app_init() {
    terminal_init();
    terminal_print("Welcome to MooseOS Terminal");
    terminal_print("Type 'help' for available commands");
}

/**
 * Check if terminal is active
 */
bool terminal_is_active() {
    return terminal_active;
}

/**
 * Open terminal application
 */
void gui_open_terminal() {
    terminal_app_init();
    gui_draw_terminal();
}