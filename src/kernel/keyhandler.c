/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "include/tty.h"
#include "include/keyboard.h"
#include "../gui/include/gui.h"
#include "../gui/include/dock.h"
#include "../gui/include/terminal.h"
#include "include/keydef.h"
#include <stdbool.h>

// external vars (dock.c)
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;

// external functions
extern bool gui_handle_dialog_key(unsigned char key, char scancode);
extern bool gui_handle_explorer_key(unsigned char key, char scancode);
extern bool gui_handle_editor_key(unsigned char key, char scancode);
extern bool dock_handle_key(unsigned char key, char scancode); 
extern bool terminal_handlekey(unsigned char key, char scancode); 
extern bool pong_handlekey(unsigned char key, char scancode);

// set 'em all to false
bool dialog_active = false;
bool explorer_active = false;
bool caps = false;
bool shift = false;

// shift is nup
static bool shift_pressed = false;

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
            // Shift + number gives symbols
            return ")!@#$%^&*("[scancode - 0x02];
        } else {
            // Just numbers
            return "1234567890"[scancode - 0x02];
        }
    }
    // space key
    else if (scancode == 0x39) {
        return ' ';
    }
    // special, special ones
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
    
    return 0; // not a character, GET OUT!!
}

void processKey(unsigned char key, char scancode) {
    if (scancode < 0)
        return;
        
    // shift
    if (scancode == LSHIFT_KEY_CODE || scancode == RSHIFT_KEY_CODE) {
        shift = !shift;
        shift_pressed = (key != 0);  // true for press, false for release
        return;
    }
    
    // caps
    if (scancode == CAPS_KEY_CODE) {
        caps = !caps;
        return;
    }
    
    // convert scancode to char
    char character = scancode_to_char(scancode, shift_pressed);
        
    // check terminal
    if (terminal_is_active()) {
        if (terminal_handlekey(character, scancode)) {  // Pass character, not key
            return;
        }
    }

    // check pong
    else if (pong_active == true) {
        if (pong_handlekey(character, scancode)) {
            return;
        }
    }

    // check editor
    else if (editor_active) {
        if (gui_handle_editor_key(character, scancode)) {
            return;
        }
    }
    // check explorer
    else if (explorer_active) {
        // explorer active, check dialog
        if (dialog_active) {
            gui_handle_dialog_key(character, scancode);
            return;
        }
        // nope just explorer
        else if (gui_handle_explorer_key(character, scancode)) {
            return;
        }
    }
    // THEN we check the dock (because yes)
    else {
        if (dock_handle_key(character, scancode)) {
            return;
        }
    }
}

