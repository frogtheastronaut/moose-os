#include "include/tty.h"
#include "include/keyboard.h"
#include "../gui/gui.h"
#include "../gui/dock.h"  // Add this include
#include "../gui/terminal.h"  // Add this include
#include <stdbool.h>

// External variables from GUI
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;

// External functions from GUI
extern bool gui_handle_dialog_key(unsigned char key, char scancode);
extern bool gui_handle_explorer_key(unsigned char key, char scancode);
extern bool gui_handle_editor_key(unsigned char key, char scancode);
extern bool gui_handle_dock_key(unsigned char key, char scancode);  // Add this
extern bool gui_handle_terminal_key(unsigned char key, char scancode); // Add this

// Keyboard scan codes
#define ENTER_KEY_CODE 0x1C
#define BS_KEY_CODE 0xE
#define CAPS_KEY_CODE 0x3A
#define LSHIFT_KEY_CODE 0x2A
#define RSHIFT_KEY_CODE 0x36
#define ESC_KEY_CODE 0x01
#define ARROW_UP_KEY 0x48
#define ARROW_DOWN_KEY 0x50
#define ARROW_LEFT_KEY 0x4B
#define ARROW_RIGHT_KEY 0x4D

bool dialog_active = false;
bool explorer_active = false;
bool caps = false;
bool shift = false;

// Add global shift state tracking
static bool shift_pressed = false;

/**
 * Maps a keyboard scancode to an ASCII character
 */
char get_char_from_scancode(unsigned char scancode, bool shift_pressed) {
    // Handle letters
    if (scancode >= 0x10 && scancode <= 0x19) {
        // QWERTYUIOP row
        char c = "qwertyuiop"[scancode - 0x10];
        return shift_pressed || caps ? c - 32 : c; // Convert to uppercase if needed
    } 
    else if (scancode >= 0x1E && scancode <= 0x26) {
        // ASDFGHJKL row
        char c = "asdfghjkl"[scancode - 0x1E];
        return shift_pressed || caps ? c - 32 : c;
    }
    else if (scancode >= 0x2C && scancode <= 0x32) {
        // ZXCVBNM row
        char c = "zxcvbnm"[scancode - 0x2C];
        return shift_pressed || caps ? c - 32 : c;
    }
    // Numbers and symbols (with shift)
    else if (scancode >= 0x02 && scancode <= 0x0B) {
        if (shift_pressed) {
            // Shift + number gives symbols
            return ")!@#$%^&*("[scancode - 0x02];
        } else {
            // Just numbers
            return "1234567890"[scancode - 0x02];
        }
    }
    // Space
    else if (scancode == 0x39) {
        return ' ';
    }
    // Special characters
    else if (scancode == 0x0C) return shift_pressed ? '_' : '-';
    else if (scancode == 0x0D) return shift_pressed ? '+' : '=';
    else if (scancode == 0x1A) return shift_pressed ? '{' : '[';
    else if (scancode == 0x1B) return shift_pressed ? '}' : ']';
    else if (scancode == 0x27) return shift_pressed ? ':' : ';';
    else if (scancode == 0x28) return shift_pressed ? '"' : '\'';
    else if (scancode == 0x29) return shift_pressed ? '~' : '`';
    else if (scancode == 0x2B) return shift_pressed ? '|' : '\\';
    else if (scancode == 0x33) return shift_pressed ? '<' : ',';
    else if (scancode == 0x34) return shift_pressed ? '>' : '.';
    else if (scancode == 0x35) return shift_pressed ? '?' : '/';
    
    return 0; // Not a typable character
}

void processKey(unsigned char key, char scancode) {
    if (scancode < 0)
        return;
        
    // Handle shift keys
    if (scancode == LSHIFT_KEY_CODE || scancode == RSHIFT_KEY_CODE) {
        shift = !shift;
        shift_pressed = (key != 0);  // true for press, false for release
        return;
    }
    
    // Handle caps lock
    if (scancode == CAPS_KEY_CODE) {
        caps = !caps;
        return;
    }
    
    // Convert scancode to ASCII if it's a typable character
    char character = get_char_from_scancode(scancode, shift_pressed);
        
    // Check terminal first (before other interfaces)
    if (terminal_is_active()) {
        if (gui_handle_terminal_key(character, scancode)) {  // Pass character, not key
            return;
        }
    }
    // Check if text editor is active
    else if (editor_active) {
        if (gui_handle_editor_key(character, scancode)) {
            return;
        }
    }
    else if (explorer_active) {
        // If dialog is active, pass both the scancode and the converted character
        if (dialog_active) {
            gui_handle_dialog_key(character, scancode);
            return;
        }
        // Otherwise let the explorer handle navigation keys
        else if (gui_handle_explorer_key(character, scancode)) {
            return;
        }
    }
    // Add dock handling - this should be the default when no other interface is active
    else {
        if (gui_handle_dock_key(character, scancode)) {
            return;
        }
    }
}

