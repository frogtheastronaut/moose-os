/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/gui.h"
#include "../kernel/include/tty.h"
#include "../kernel/include/keyboard.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../lib/lib.h"
#include "../time/rtc.h"

// externs
extern void copyStr(char* dest, const char* src);
extern int strEqual(const char* a, const char* b);
extern int msnprintf(char *buffer, int size, const char *format, ...);
extern size_t strlen(const char* str);
extern char* strcat(char* dest, const char* src);
extern const char* strip_whitespace(const char* str);
extern void dock_return(void); 

// defs
#define TERMINAL_WIDTH 300
#define TERMINAL_HEIGHT 180
#define TERMINAL_X ((SCREEN_WIDTH - TERMINAL_WIDTH) / 2)
#define TERMINAL_Y ((SCREEN_HEIGHT - TERMINAL_HEIGHT) / 2)

// display area
#define TERM_AREA_X (TERMINAL_X + 8)
#define TERM_AREA_Y (TERMINAL_Y + 28)
#define TERM_AREA_WIDTH (TERMINAL_WIDTH - 16)
#define TERM_AREA_HEIGHT (TERMINAL_HEIGHT - 36)

// colors
#define TERM_BG_COLOR VGA_COLOR_BLACK
#define TERM_TEXT_COLOR VGA_COLOR_WHITE
#define TERM_PROMPT_COLOR VGA_COLOR_GREEN
#define TERM_ERROR_COLOR VGA_COLOR_RED

// settings
#define MAX_COMMAND_LEN 64
#define MAX_LINES 15
#define CHARS_PER_LINE 35

static char command_buffer[MAX_COMMAND_LEN + 1] = "";
static int command_pos = 0;
static char terminal_lines[MAX_LINES][CHARS_PER_LINE + 1];
static int current_line = 0;
static int scroll_offset = 0;
static bool terminal_active = false;

// more externs. 
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
extern File* root;
extern File* cwd;

extern void draw_cursor(void);

void terminal_init() {
    for (int i = 0; i < MAX_LINES; i++) {
        terminal_lines[i][0] = '\0';
    }
    current_line = 0;
    scroll_offset = 0;
    command_buffer[0] = '\0';
    command_pos = 0;
}

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
    
    // new line
    int i = 0;
    while (i < CHARS_PER_LINE && text[i] != '\0') {
        terminal_lines[current_line][i] = text[i];
        i++;
    }
    terminal_lines[current_line][i] = '\0';
    current_line++;
}

/**
 * print text
 */
static void terminal_print(const char* text) {
    terminal_add_line(text, TERM_TEXT_COLOR);
}

/**
 * error msg
 */
static void terminal_print_error(const char* text) {
    terminal_add_line(text, TERM_ERROR_COLOR);
}

/**
 * get cwd name
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
 * executs command
 */
static void term_exec_cmd(const char* cmd) {
    // strip whitespace
    cmd = strip_whitespace(cmd);
    
    // add cmd to history
    char prompt_line[CHARS_PER_LINE + 1];
    msnprintf(prompt_line, sizeof(prompt_line), "%s# %s", get_cwd(), cmd); 
    terminal_add_line(prompt_line, TERM_PROMPT_COLOR);
    
    if (strlen(cmd) == 0) {
        return; // empty
    }
    // help
    if (strEqual(cmd, "help")) {
        terminal_print("Welcome to the MooseOS Terminal");
        terminal_print("help - Show this help");
        terminal_print("ls - List files");
        terminal_print("cd <dir> - Change directory");
        terminal_print("mkdir <name> - Create directory");
        terminal_print("touch <name> - Create file");
        terminal_print("cat <file> - Show file content");
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
        terminal_init();
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
        // year
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
    // unknown
    else {
        char line[CHARS_PER_LINE + 1];
        msnprintf(line, sizeof(line), "Unknown command: %s", cmd);
        terminal_print_error(line);
        terminal_print("Type 'help' for commands");
    }
}

/**
 * draw terminal window
 */
static void terminal_draw_win() {
    gui_clear(VGA_COLOR_LIGHT_GREY);
    draw_windowbox(TERMINAL_X, TERMINAL_Y, TERMINAL_WIDTH, TERMINAL_HEIGHT,
                       VGA_COLOR_BLACK, VGA_COLOR_WHITE, VGA_COLOR_LIGHT_GREY);
    draw_title(TERMINAL_X, TERMINAL_Y, TERMINAL_WIDTH, 15, VGA_COLOR_BLUE);
    draw_text(TERMINAL_X + 5, TERMINAL_Y + 3, "MooseOS Terminal", VGA_COLOR_WHITE);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOR);
    draw_rectoutline(TERM_AREA_X - 1, TERM_AREA_Y - 1, TERM_AREA_WIDTH + 2, TERM_AREA_HEIGHT + 2, VGA_COLOR_DARK_GREY);
}

/**
 * draw terminal content
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
    draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOR);
}



/**
 * draw terminal
 */
void draw_term() {
    gui_init();
    terminal_draw_win();
    term_draw_content();
    
    terminal_active = true;
    dialog_active = false;
    explorer_active = false;
    editor_active = false;
    draw_cursor();
}

/**
 * handle terminal keyboard input
 */
bool term_handlekey(unsigned char key, char scancode) {
    if (!terminal_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            // exit
            terminal_active = false;
            dock_return();
            return true;
            
        case ENTER_KEY_CODE:
            // exec
            term_exec_cmd(command_buffer);
            command_buffer[0] = '\0';
            command_pos = 0;
            draw_term(); 
            return true;
            
        case BS_KEY_CODE:
            // backspace
            if (command_pos > 0) {
                command_pos--;
                command_buffer[command_pos] = '\0';
                draw_term(); 
            }
            return true;
            
        default:
            // 'normal characters'
            if (key >= 32 && key < 127 && command_pos < MAX_COMMAND_LEN) {
                command_buffer[command_pos] = key;
                command_pos++;
                command_buffer[command_pos] = '\0';
                draw_term(); 
                return true; 
            } 
    }
}

/**
 * init
 */
void term_init() {
    terminal_init();
    terminal_print("Welcome to MooseOS Terminal");
    terminal_print("Type 'help' for available commands");
}

/**
 * terminal_active but a function
 */
bool term_isactive() {
    return terminal_active;
}

/**
 * open term
 */
void gui_open_terminal() {
    term_init();
    draw_term();
}