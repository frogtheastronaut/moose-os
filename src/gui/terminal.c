
/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang.
*/

#include "include/terminal.h"
#include "../kernel/include/disk.h"

static char command_buffer[MAX_COMMAND_LEN + 1] = "";
static int command_pos = 0;
static char terminal_lines[MAX_LINES][CHARS_PER_LINE + 1];
static int current_line = 0;
bool terminal_active = false;

/**
 * Clear the terminal
 * Used in the terminal `clear` function
 */
void clear_terminal() {
    for (int i = 0; i < MAX_LINES; i++) {
        terminal_lines[i][0] = '\0';
    }
    current_line = 0;
    command_buffer[0] = '\0';
    command_pos = 0;
}

/**
 * Add a line to the terminal
 */
static void terminal_add_line(const char* text, uint8_t color) {
    if (current_line >= MAX_LINES) {
        // scroll up
        for (int i = 0; i < MAX_LINES - 1; i++) {
            int j = 0;
            while (j < CHARS_PER_LINE && terminal_lines[i + 1][j] != '\0') {
                terminal_lines[i][j] = terminal_lines[i + 1][j];
                j++;
            }
            terminal_lines[i][j] = '\0';
        }
        current_line = MAX_LINES - 1;
    }
    
    // New line
    int i = 0;
    while (i < CHARS_PER_LINE && text[i] != '\0') {
        terminal_lines[current_line][i] = text[i];
        i++;
    }
    terminal_lines[current_line][i] = '\0';
    current_line++;
}

/**
 * Print text
 */
static void terminal_print(const char* text) {
    terminal_add_line(text, TERM_TEXT_COLOR);
}

/**
 * Print error message
 */
static void terminal_print_error(const char* text) {
    terminal_add_line(text, TERM_ERROR_COLOR);
}

/**
 * Get current working directory name
 */
static const char* get_cwd() {
    if (cwd == root) {
        return "/";
    }
    static char dir_with_slash[MAX_NAME_LEN + 2]; 
    msnprintf(dir_with_slash, sizeof(dir_with_slash), "%s/", cwd->name);
    return dir_with_slash;
}

/**
 * Execute a command
 * 
 * @todo: Add tokenization + other features commonly found in a shell.
 *        This is high priority.
 */
static void term_exec_cmd(const char* cmd) {
    // Strip whitespace
    cmd = strip_whitespace(cmd);
    
    // Add cmd to history
    char prompt_line[CHARS_PER_LINE + 1];
    msnprintf(prompt_line, sizeof(prompt_line), "%s# %s", get_cwd(), cmd); 
    terminal_add_line(prompt_line, TERM_PROMPT_COLOR);
    
    if (strlen(cmd) == 0) {
        return; // empty
    }
    // help
    /**
     * @todo: Double check this
     */
    if (strEqual(cmd, "help")) {
        terminal_print("Welcome to the MooseOS Terminal");
        terminal_print("ls - List files");
        terminal_print("cd <dir> - Change directory");
        terminal_print("mkdir <name> - Create directory");
        terminal_print("touch <name> - Create file");
        terminal_print("cat <file> - Show file content");
        terminal_print("diskinfo - Show disk information");
        terminal_print("memstats - Show memory statistics");
        terminal_print("save - Save current filesystem to disk");
        terminal_print("load - Load filesystem from disk");
        terminal_print("disktest - Test basic disk read/write");
        terminal_print("help - Show this help");
        terminal_print("clear - Clear terminal");
    }
    // ls
    else if (strEqual(cmd, "ls")) {
        if (cwd->folder.childCount == 0) {
            terminal_print("Directory is empty.");
        } else {
            for (int i = 0; i < cwd->folder.childCount; i++) {
                File* child = cwd->folder.children[i];
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
    // cd
    else if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
        const char* dirname = cmd + 3;
        if (strEqual(dirname, "..")) {
            if (cwd->parent) {
                cwd = cwd->parent;
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Changed to %s", get_cwd());
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
                File* child = cwd->folder.children[i];
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
    // mkdir
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
    // touch
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
    // cat
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
        const char* filename = cmd + 4;
        
        // Search for the file in current directory
        bool found = false;
        for (int i = 0; i < cwd->folder.childCount; i++) {
            File* child = cwd->folder.children[i];
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
    // clear term
    else if (strEqual(cmd, "clear")) {
        clear_terminal();
    }
    // show time/date
    else if (strEqual(cmd, "time")) {
        rtc_time local_time = rtc_gettime();
        char time_line[CHARS_PER_LINE + 1];
        char date_line[CHARS_PER_LINE + 1];
        
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
        
        // Format date (DD/MM/YYYY)
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

        // Year
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
        
        msnprintf(time_line, sizeof(time_line), "Time: %s:%s:%s", hour_str, min_str, sec_str);
        terminal_print(time_line);
        msnprintf(date_line, sizeof(date_line), "Date: %s/%s/%s", day_str, month_str, year_str);
        terminal_print(date_line);
    }
    // settimezone - set UTC+X
    else if (cmd[0] == 's' && cmd[1] == 'e' && cmd[2] == 't' && cmd[3] == 't' && cmd[4] == 'i' && 
             cmd[5] == 'm' && cmd[6] == 'e' && cmd[7] == 'z' && cmd[8] == 'o' && cmd[9] == 'n' && 
             cmd[10] == 'e' && cmd[11] == ' ') {
        const char* offset_str = cmd + 12;
        if (strlen(offset_str) > 0) {
            int offset = 0;
            int sign = 1;
            int i = 0;
            
            if (offset_str[0] == '-') {
                sign = -1;
                i = 1;
            } else if (offset_str[0] == '+') {
                i = 1;
            }
            
            while (offset_str[i] >= '0' && offset_str[i] <= '9') {
                offset = offset * 10 + (offset_str[i] - '0');
                i++;
            }
            
            offset *= sign;
            
            if (offset >= -12 && offset <= 14) {
                timezone_offset = offset; 
                terminal_print("Timezone updated");
            } else {
                timezone_offset = offset;
                terminal_print_error("Wierd timezone, but sure");
            }
        } else {
            terminal_print_error("Usage: settimezone <hours>");
        }
    }
    
    else if (strEqual(cmd, "diskinfo")) {
        char info_buffer[512];
        if (filesys_get_disk_info(info_buffer, sizeof(info_buffer)) == 0) {
            char *line_start = info_buffer;
            char *line_end;
            
            while (*line_start) {
                line_end = line_start;
                // Find end of line or end of string
                while (*line_end && *line_end != '\n') {
                    line_end++;
                }
                
                // Create null-terminated line
                char temp_char = *line_end;
                *line_end = '\0';
                terminal_print(line_start);
                *line_end = temp_char;
                
                // Move to next line
                if (*line_end == '\n') {
                    line_end++;
                }
                line_start = line_end;
            }
        } else {
            terminal_print_error("Failed to get disk information");
        }
    }
    
    else if (strEqual(cmd, "memstats")) {
        char stats_buffer[512];
        if (filesys_get_memory_stats(stats_buffer, sizeof(stats_buffer)) == 0) {
            char *line_start = stats_buffer;
            char *line_end;
            
            while (*line_start) {
                line_end = line_start;
                // Find end of line or end of string
                while (*line_end && *line_end != '\n') {
                    line_end++;
                }
                
                // Create null-terminated line
                char temp_char = *line_end;
                *line_end = '\0';
                terminal_print(line_start);
                *line_end = temp_char;
                
                // Move to next line
                if (*line_end == '\n') {
                    line_end++;
                }
                line_start = line_end;
            }
        } else {
            terminal_print_error("Failed to get memory statistics");
        }
    }

    // save - save current filesystem to disk
    else if (strEqual(cmd, "save")) {
        if (filesys_disk_status()) {
            terminal_print("Saving filesystem to disk...");
            
            // First try to sync
            int sync_result = filesys_sync();
            if (sync_result != 0) {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Sync failed with error: %d", sync_result);
                terminal_print_error(line);
            } else {
                terminal_print("Superblock synced successfully");
            }
            
            // Then save filesystem data
            int save_result = filesys_save_to_disk();
            if (save_result == 0) {
                terminal_print("Filesystem data saved successfully");
                
                // Force cache flush
                filesys_flush_cache();
                terminal_print("Cache flushed to disk");
                
                terminal_print("=== SAVE COMPLETE ===");
            } else {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Save failed with error: %d", save_result);
                terminal_print_error(line);
            }
        } else {
            terminal_print_error("No filesystem mounted");
        }
    }
    
    // load - load filesystem from disk
    else if (strEqual(cmd, "load")) {
        if (filesys_disk_status()) {
            if (filesys_load_from_disk() == 0) {
                terminal_print("Filesystem loaded from disk successfully");
            } else {
                terminal_print_error("Failed to load filesystem from disk");
            }
        } else {
            terminal_print_error("No filesystem mounted");
        }
    }
    
    // disktest - test disk operations
    else if (strEqual(cmd, "disktest")) {
        terminal_print("Commencing disk test...");
        
        extern ata_device_t ata_devices[4];
        if (!ata_devices[0].exists) {
            terminal_print_error("FATAL: No disk device found!");
            return;
        }
        
        char line[CHARS_PER_LINE + 1];
        uint8_t test_buffer[512];
        
        // Simple test: write pattern, read back, verify
        // Fill with simple repeating pattern
        for (int i = 0; i < 512; i++) {
            test_buffer[i] = (uint8_t)(i % 256);
        }
        
        // Write to sector 100
        terminal_print("Writing...");
        int write_result = disk_write_sector(0, 100, test_buffer);
        msnprintf(line, sizeof(line), "Write result: %d", write_result);
        terminal_print(line);
        
        // Clear buffer
        for (int i = 0; i < 512; i++) {
            test_buffer[i] = 0xCC;
        }
        
        // Read back
        terminal_print("Reading...");
        int read_result = disk_read_sector(0, 100, test_buffer);
        msnprintf(line, sizeof(line), "Read result: %d", read_result);
        terminal_print(line);
        
        // Show first 8 bytes
        msnprintf(line, sizeof(line), "Data: %02X %02X %02X %02X %02X %02X %02X %02X",
            test_buffer[0], test_buffer[1], test_buffer[2], test_buffer[3],
            test_buffer[4], test_buffer[5], test_buffer[6], test_buffer[7]);
        terminal_print(line);
        
        // Simple check: are first few bytes what we expect?
        if (test_buffer[0] == 0 && test_buffer[1] == 1 && test_buffer[2] == 2) {
            terminal_print("SUCCESS: Data matches!");
        } else {
            terminal_print("Data mismatch or corruption!");
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
static void terminal_draw_win() {
    gui_clear(VGA_COLOR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOR);
}

/**
 * Draw terminal content
 */
static void term_draw_content() {
    int y_pos = TERM_AREA_Y + 5;
    int visible_lines = (TERM_AREA_HEIGHT - 35) / 10; 
    
    int start_line = (current_line > visible_lines) ? current_line - visible_lines : 0;
    for (int i = start_line; i < current_line && i < start_line + visible_lines; i++) {
        if (terminal_lines[i][0] != '\0') {
            draw_text(TERM_AREA_X + 5, y_pos, terminal_lines[i], TERM_TEXT_COLOR);
            y_pos += 10;
        }
    }
    
    char prompt[CHARS_PER_LINE + 1];
    msnprintf(prompt, sizeof(prompt), "%s# %s", get_cwd(), command_buffer); 
    draw_text(TERM_AREA_X + 5, y_pos, prompt, TERM_PROMPT_COLOR);
    
    int cursor_x = TERM_AREA_X + 5 + get_textwidth(prompt);

    /**
     * Currently, the cursor is a _
     * 
     * @todo: add different types of cursors. This is low priority.
     */

    draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOR);
}

/**
 * Optimized function to only redraw the prompt line (much faster for typing)
 */
static void term_redraw_prompt_only() {
    // Calculate prompt line position
    int y_pos = TERM_AREA_Y + 5;
    int visible_lines = (TERM_AREA_HEIGHT - 35) / 10;
    int start_line = (current_line > visible_lines) ? current_line - visible_lines : 0;
    
    // Skip to prompt line position
    for (int i = start_line; i < current_line; i++) {
        if (terminal_lines[i][0] != '\0') {
            y_pos += 10;
        }
    }
    
    // Clear the prompt line area (overwrite with background color)
    draw_rect(TERM_AREA_X + 5, y_pos, TERM_AREA_WIDTH - 10, 10, TERM_BG_COLOR);
    
    // Redraw only the prompt and cursor
    char prompt[CHARS_PER_LINE + 1];
    msnprintf(prompt, sizeof(prompt), "%s# %s", get_cwd(), command_buffer); 
    draw_text(TERM_AREA_X + 5, y_pos, prompt, TERM_PROMPT_COLOR);
    
    int cursor_x = TERM_AREA_X + 5 + get_textwidth(prompt);
    draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOR);
}



/**
 * Draw terminal
 */
void draw_term() {
    // Always ensure background is drawn first
    gui_clear(VGA_COLOR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOR);
    
    // Set terminal state
    terminal_active = true;
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
    
    term_draw_content();
    draw_cursor();
}

/**
 * Handle terminal keyboard input
 */
bool term_handlekey(unsigned char key, char scancode) {
    if (!terminal_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            terminal_active = false;
            dock_return();
            return true;
            
        case ENTER_KEY_CODE:
            // Enter key executes commands
            term_exec_cmd(command_buffer);
            command_buffer[0] = '\0';
            command_pos = 0;
            draw_term(); // Full redraw needed after command execution
            return true;
            
        case BS_KEY_CODE:
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                term_redraw_prompt_only(); // Only redraw the prompt line
            }
            return true;
            
        default:
            // Printable characters
            if (key >= 32 && key < 127 && command_pos < MAX_COMMAND_LEN) {
                command_buffer[command_pos] = key;
                command_pos++;
                command_buffer[command_pos] = '\0';
                term_redraw_prompt_only(); // Only redraw the prompt line
                return true; 
            } 
    }
}

/**
 * Initialize terminal
 */
void term_init() {
    clear_terminal();
    
    // Draw the background immediately when initializing
    gui_clear(VGA_COLOR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOR);
    
    terminal_print("Welcome to MooseOS Terminal");
    terminal_print("Type 'help' for available commands");
    terminal_print("Press [ESC] to exit");
    
    // Ensure the terminal is ready to be drawn
    draw_term();
}

/**
 * Check if terminal is active
 */
bool term_isactive() {
    return terminal_active;
}

/**
 * Open terminal
 */
void gui_open_terminal() {
    term_init();
    draw_term();
}