/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "keyhandler.h"
#include <stdbool.h>
static bool shift_pressed = false;

bool dialog_active = false;
bool explorer_active = false;
bool editor_active = false;
static bool caps = false;

char scancode_to_char(unsigned char scancode, bool shift_pressed) {
    // QWERTYUIOP row
    if (scancode >= Q_KEY_CODE && scancode <= P_KEY_CODE) {
        char c = "qwertyuiop"[scancode - Q_KEY_CODE];
        return (shift_pressed || caps) ? c - 32 : c;
    }
    // ASDFGHJKL row
    else if (scancode >= A_KEY_CODE && scancode <= L_KEY_CODE) {
        char c = "asdfghjkl"[scancode - A_KEY_CODE];
        return (shift_pressed || caps) ? c - 32 : c;
    }
    // ZXCVBNM row
    else if (scancode >= Z_KEY_CODE && scancode <= M_KEY_CODE) {
        char c = "zxcvbnm"[scancode - Z_KEY_CODE];
        return (shift_pressed || caps) ? c - 32 : c;
    }
    // Numbers
    else if (scancode >= NUM_1_KEY_CODE && scancode <= NUM_0_KEY_CODE) {
        if (shift_pressed) {
            return ")!@#$%^&*("[scancode - NUM_1_KEY_CODE];
        } else {
            return "1234567890"[scancode - NUM_1_KEY_CODE];
        }
    }
    // Space key
    else if (scancode == SPACE_KEY_CODE) {
        return ' ';
    }
    // Symbols
    else if (scancode == MINUS_KEY_CODE) return shift_pressed ? '_' : '-';
    else if (scancode == EQUAL_KEY_CODE) return shift_pressed ? '+' : '=';
    else if (scancode == LBRACKET_KEY_CODE) return shift_pressed ? '{' : '[';
    else if (scancode == RBRACKET_KEY_CODE) return shift_pressed ? '}' : ']';
    else if (scancode == SEMICOLON_KEY_CODE) return shift_pressed ? ':' : ';';
    else if (scancode == APOSTROPHE_KEY_CODE) return shift_pressed ? '"' : '\'';
    else if (scancode == GRAVE_KEY_CODE) return shift_pressed ? '~' : '`';
    else if (scancode == BACKSLASH_KEY_CODE) return shift_pressed ? '|' : '\\';
    else if (scancode == COMMA_KEY_CODE) return shift_pressed ? '<' : ',';
    else if (scancode == DOT_KEY_CODE) return shift_pressed ? '>' : '.';
    else if (scancode == SLASH_KEY_CODE) return shift_pressed ? '?' : '/';

    return 0; // not a character
}

void processKey(unsigned char key, char scancode) {
    unsigned char raw_scancode = (unsigned char)scancode;
    bool key_released = (raw_scancode & 0x80) != 0;
    unsigned char base_scancode = raw_scancode & 0x7F; 
        
    // Shift
    if (base_scancode == LSHIFT_KEY_CODE || base_scancode == RSHIFT_KEY_CODE) {
        if (key_released) {
            shift_pressed = false;
        } else {
            shift_pressed = true;
        }
        return;
    }
    
    // Don't process release keys
    if (key_released) {
        return;
    }
    
    // Caps lock
    if (base_scancode == CAPS_KEY_CODE) {
        caps = !caps;
        return;
    }
    
    // Convert scancode to char
    char character = scancode_to_char(base_scancode, shift_pressed);
        
    /**
     * @todo Currently, the logic states that only
     * terminal, gui, dialog and a few more apps can receive
     * keys. If we want custom apps, we would need to change the logic.
     */
    // Check if terminal is active
    if (term_isactive()) {
        if (term_handlekey(character, base_scancode)) {
            return;
        }
    }

    // Check editor
    else if (editor_active) {
        if (gui_handle_editor_key(character, base_scancode)) {
            return;
        }
    }
    // Check explorer
    else if (explorer_active) {
        // Explorer active, check dialog
        if (dialog_active) {
            gui_handle_dialog_input(character, base_scancode);
            return;
        }
        // Just explorer active
        else if (gui_handle_explorer_key(character, base_scancode)) {
            return;
        }
    }
    // Then check dock last
    else {
        if (dock_handle_key(character, base_scancode)) {
            return;
        }
    }
}

