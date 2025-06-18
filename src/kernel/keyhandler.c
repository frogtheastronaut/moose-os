#include "include/tty.h"
#include "include/keyboard.h"
#include <stdbool.h>

#define ENTER_KEY_CODE 0x1C
#define BS_KEY_CODE 0xE
#define CAPS_KEY_CODE 0x3A
bool caps = false;

void debug(unsigned char key, char keycode) {
	if(keycode < 0)
		return;
	if(keycode == ENTER_KEY_CODE) {
		terminal_newline();
		return;
	} else if (keycode == BS_KEY_CODE) {
		terminal_backspace();
		return;
	} else if (keycode == CAPS_KEY_CODE) {
		if (caps == true)  {
			caps = false;
		} else {
			caps = true;
		}
		return;
	} else{
		if (caps == false) {
			terminal_putchar(keyboard_map_normal[(unsigned char) keycode]);
		} else {
			terminal_putchar(keyboard_map_caps[(unsigned char) keycode]);
		}
	}
}

void processKey(unsigned char key, char keycode) {
	// This function forwards the key pressed to other programs.
	// It is not the Kernel's responsibility to decide what to do with the key recieved.
	// As a result, we forward it to the OS. Farewell!

	// As a debug, we can always print the key to the kernel.
	debug(key, keycode);
}

