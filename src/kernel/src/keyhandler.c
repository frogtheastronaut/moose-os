/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "../include/keyhandler.h"
static bool shift_pressed = false;

bool dialog_active = false;
bool explorer_active = false;
bool editor_active = false;
bool caps = false;

// turn a scancode into a char
char scancode_to_char(unsigned char scancode, bool shift_pressed) {
    // typable letters
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
    // numbers with or w/o shift
    else if (scancode >= 0x02 && scancode <= 0x0B) {
        if (shift_pressed) {
            // symbol
            return ")!@#$%^&*("[scancode - 0x02];
        } else {
            // numbers
            return "1234567890"[scancode - 0x02];
        }
    }
    // space key
    else if (scancode == 0x39) {
        return ' ';
    }
    // misc
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
    
    return 0; // not a character 
}

void processKey(unsigned char key, char scancode) {
    unsigned char raw_scancode = (unsigned char)scancode;
    bool key_released = (raw_scancode & 0x80) != 0;
    unsigned char base_scancode = raw_scancode & 0x7F; 
        
    // shift - handle press and release properly
    if (base_scancode == LSHIFT_KEY_CODE || base_scancode == RSHIFT_KEY_CODE) {
        if (key_released) {
            shift_pressed = false;
        } else {
            shift_pressed = true;
        }
        return;
    }
    
    // dont process release keys
    if (key_released) {
        return;
    }
    
    // caps lock - only toggle on key press, not release
    if (base_scancode == CAPS_KEY_CODE) {
        caps = !caps;
        return;
    }
    
    // convert scancode to char using base scancode
    char character = scancode_to_char(base_scancode, shift_pressed);
        
    // check terminal
    if (term_isactive()) {
        if (term_handlekey(character, base_scancode)) {
            return;
        }
    }

    // check editor
    else if (editor_active) {
        if (gui_handle_editor_key(character, base_scancode)) {
            return;
        }
    }
    // check explorer
    else if (explorer_active) {
        // explorer active, check dialog
        if (dialog_active) {
            gui_handle_dialog_input(character, base_scancode);
            return;
        }
        // nope just explorer
        else if (gui_handle_explorer_key(character, base_scancode)) {
            return;
        }
    }
    // THEN we check the dock (because yes)
    else {
        if (dock_handle_key(character, base_scancode)) {
            return;
        }
    }
}

