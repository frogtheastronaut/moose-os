/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "terminal.h"
#include "ata/ata.h"
#include "paging/paging.h"
#include "terminal_cmd.h"

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
static void terminal_add_line(const char* text, uint8_t colour) {
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
 * Wrap text and add multiple lines to terminal if needed
 */
void terminal_add_wrapped_text(const char* text, uint8_t colour) {
    if (!text) return;
    
    int text_len = strlen(text);
    char line_buffer[CHARS_PER_LINE + 1];
    int pos = 0;
    
    for (int i = 0; i <= text_len; i++) {
        // If we hit end of string or newline, flush current line
        if (text[i] == '\0' || text[i] == '\n') {
            line_buffer[pos] = '\0';
            if (pos > 0) {
                terminal_add_line(line_buffer, colour);
            }
            pos = 0;
            continue;
        }
        
        // If line is getting too long, wrap it
        if (pos >= CHARS_PER_LINE) {
            line_buffer[pos] = '\0';
            terminal_add_line(line_buffer, colour);
            pos = 0;
        }
        
        line_buffer[pos++] = text[i];
    }
}

/**
 * Print text with wrapping
 */
void terminal_print(const char* text) {
    terminal_add_wrapped_text(text, TERM_TEXT_COLOUR);
}

/**
 * Print error message with wrapping
 */
void terminal_print_error(const char* text) {
    terminal_add_wrapped_text(text, TERM_ERROR_COLOUR);
}

/**
 * Get current working directory name
 */
const char* get_cwd() {
    if (cwd == root) {
        return "/";
    }
    static char dir_with_slash[MAX_NAME_LEN + 2]; 
    msnprintf(dir_with_slash, sizeof(dir_with_slash), "%s/", cwd->name);
    return dir_with_slash;
}

/**
 * Draw terminal window
 */
static void terminal_draw_win() {
    gui_clear(VGA_COLOUR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOUR);
}

/**
 * Draw terminal content
 */
static void term_draw_content() {
    int y_pos = TERM_AREA_Y + FONT_SPACING;
    int visible_lines = (TERM_AREA_HEIGHT - 35) / FONT_HEIGHT;
    
    // Ensure we don't show more lines than we have stored
    if (visible_lines > MAX_LINES) {
        visible_lines = MAX_LINES;
    } 
    
    int start_line = (current_line > visible_lines) ? current_line - visible_lines : 0;
    for (int i = start_line; i < current_line && i < start_line + visible_lines; i++) {
        if (terminal_lines[i][0] != '\0') {
            draw_text(TERM_AREA_X + FONT_SPACING, y_pos, terminal_lines[i], TERM_TEXT_COLOUR);
            y_pos += FONT_HEIGHT;
        }
    }
    
    char prompt[CHARS_PER_LINE + 1];
    msnprintf(prompt, sizeof(prompt), "%s# %s", get_cwd(), command_buffer); 
    draw_text(TERM_AREA_X + FONT_SPACING, y_pos, prompt, TERM_PROMPT_COLOUR);
    
    int cursor_x = TERM_AREA_X + FONT_SPACING + draw_text_width(prompt);

    /**
     * Currently, the cursor is a _
     * 
     * @todo: add different types of cursors. This is low priority.
     */

    draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOUR);
}

/**
 * Optimized function to only redraw the prompt line (much faster for typing)
 */
static void term_redraw_prompt_only() {
    // Calculate prompt line position
    int y_pos = TERM_AREA_Y + FONT_SPACING;
    int visible_lines = (TERM_AREA_HEIGHT - 35) / FONT_HEIGHT;
    
    // Ensure we don't show more lines than we have stored
    if (visible_lines > MAX_LINES) {
        visible_lines = MAX_LINES;
    }
    int start_line = (current_line > visible_lines) ? current_line - visible_lines : 0;
    
    // Skip to prompt line position
    for (int i = start_line; i < current_line; i++) {
        if (terminal_lines[i][0] != '\0') {
            y_pos += FONT_HEIGHT;
        }
    }
    
    // Clear the prompt line area (overwrite with background colour)
    draw_rect(TERM_AREA_X + FONT_SPACING, y_pos, TERM_AREA_WIDTH - (2 * FONT_SPACING), FONT_HEIGHT, TERM_BG_COLOUR);
    
    // Redraw only the prompt and cursor
    char prompt[CHARS_PER_LINE + 1];
    msnprintf(prompt, sizeof(prompt), "%s# %s", get_cwd(), command_buffer); 
    draw_text(TERM_AREA_X + FONT_SPACING, y_pos, prompt, TERM_PROMPT_COLOUR);
    
    int cursor_x = TERM_AREA_X + FONT_SPACING + draw_text_width(prompt);
    draw_text(cursor_x, y_pos, "_", TERM_PROMPT_COLOUR);
}



/**
 * Draw terminal
 */
void draw_term() {
    // Always ensure background is drawn first
    gui_clear(VGA_COLOUR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOUR);
    
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
bool terminal_handle_key(unsigned char key, char scancode) {
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
    gui_clear(VGA_COLOUR_LIGHT_GREY);
    draw_rect(TERM_AREA_X, TERM_AREA_Y, TERM_AREA_WIDTH, TERM_AREA_HEIGHT, TERM_BG_COLOUR);
    
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