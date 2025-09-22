#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include "keyboard_scan_codes.h"
#include "gui/gui.h"
#include "dock.h"
#include "terminal.h"
#include "keyboard/scancode_map.h"
#include "libc/lib.h"
#include "keyboard/scancode_map.h"

// external vars (dock.c)
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;

// external functions
extern bool gui_handle_dialog_input(unsigned char key, char scancode);
extern bool gui_handle_explorer_key(unsigned char key, char scancode);
extern bool gui_handle_editor_key(unsigned char key, char scancode);
extern bool dock_handle_key(unsigned char key, char scancode); 
extern bool term_handlekey(unsigned char key, char scancode);

// set 'em all to false
extern bool dialog_active;
extern bool explorer_active;
extern bool editor_active;
void processKey(unsigned char key, char scancode);

#endif
