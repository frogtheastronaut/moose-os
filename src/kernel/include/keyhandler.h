#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include "keyboard.h"
#include "../../gui/include/gui.h"
#include "../../gui/include/dock.h"
#include "../../gui/include/terminal.h"
#include "keydef.h"

#ifndef __cplusplus
#ifndef bool
#define bool _Bool
#define true 1
#define false 0
#endif
#endif

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
extern bool caps;

void processKey(unsigned char key, char scancode);

#endif
