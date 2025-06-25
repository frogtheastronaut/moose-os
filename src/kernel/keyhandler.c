#include "include/tty.h"
#include "include/keyboard.h"
#include "../gui/gui.h"
#include <stdbool.h>

// Keyboard scan codes

bool caps = false;
bool shift = false;

int arg_pos = 0;
char arg[256];
void processKey(unsigned char key, char keycode) {
	if(keycode < 0)
		return;
		
	if (explorer_active) {
		gui_handle_explorer_key(key, keycode);
		return;
	}

	// if (keycode == ESC_KEY_CODE) {
	// 	return; // Currently ignore TODO: Implemennt something else here lmao
	// }

	// if (keycode == LSHIFT_KEY_CODE || keycode == RSHIFT_KEY_CODE) {
	// 	if (shift == true) {
	// 		shift = false;
	// 	} else {
	// 		shift = true;
	// 	}
    //     return;
    // }
	// if(keycode == ENTER_KEY_CODE) {
	// 	terminal_newline();
	// 	//shell_process_command(arg);

	// 	// Reset everything
	// 	arg_pos = 0;
	// 	for(int i = 0; i < 256; i++) {
	// 		arg[i] = '\0';
	// 	}
	// 	terminal_newline();
	// 	return;
	// } else if (keycode == BS_KEY_CODE) {
	// 	terminal_backspace();
	// 	arg_pos--;
	// 	if (arg_pos < 0) {
	// 		arg_pos = 0;
	// 	}
	// 	arg[arg_pos] = '\0';
	// 	return;
	// } else if (keycode == CAPS_KEY_CODE) {
	// 	if (caps == true)  {
	// 		caps = false;
	// 	} else {
	// 		caps = true;
	// 	}
	// 	return;
	// } else{
    //     unsigned char ch = '\0';
    //     if (shift) {
    //         ch = keyboard_map_shift[(unsigned char)keycode];
    //     } else if (caps) {
    //         ch = keyboard_map_caps[(unsigned char)keycode];
    //     } else {
    //         ch = keyboard_map_normal[(unsigned char)keycode];
    //     }
    //     if (ch) {
    //         terminal_putchar(ch);
    //         arg[arg_pos++] = ch;
    //     }
	//}
}

