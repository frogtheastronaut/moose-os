#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include "keyboard.h"
#include "../../gui/include/gui.h"
#include "../../gui/include/dock.h"
#include "../../gui/include/terminal.h"
#include "scancode_map.h"
#include "../../lib/include/lib.h"

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
