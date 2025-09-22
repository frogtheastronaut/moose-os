#include "../include/keyboard.h"

void kb_init(void)
{
	write_port(0x21 , 0xF9);
	write_port(0xA1 , 0xEF);  
}

void keyboard_handler_main(void)
{

    unsigned char status;
    char keycode;

    /* write EOI */
    write_port(0x20, 0x20);

    status = read_port(KEYBOARD_STATUS_PORT);
    if (status & 0x01) {
        keycode = read_port(KEYBOARD_DATA_PORT);
        last_keycode = keycode;
        key_pressed = true;
    }

}