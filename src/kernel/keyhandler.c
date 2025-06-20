#include "include/tty.h"
#include "include/keyboard.h"
#include "../shell/shell.h"
#include <stdbool.h>

#define ENTER_KEY_CODE 0x1C
#define BS_KEY_CODE 0xE
#define CAPS_KEY_CODE 0x3A
bool caps = false;

int arg_pos = 0;
char arg[256];
void processKey(unsigned char key, char keycode) {
	if(keycode < 0)
		return;
	if(keycode == ENTER_KEY_CODE) {
		terminal_newline();
		shell_process_command(arg);

		// Reset everything
		arg_pos = 0;
		for(int i = 0; i < 256; i++) {
			arg[i] = '\0';
		}
		terminal_newline();
		shell_prompt();
		return;
	} else if (keycode == BS_KEY_CODE) {
		terminal_backspace();
		arg[arg_pos - 1] = '\0'; // Null-terminate the string
		arg_pos--;
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
			arg[arg_pos] = keyboard_map_normal[(unsigned char) keycode];
			arg_pos++;
		} else {
			terminal_putchar(keyboard_map_caps[(unsigned char) keycode]);
			arg[arg_pos] = keyboard_map_caps[(unsigned char) keycode];
			arg_pos++;
		}
	}
}

