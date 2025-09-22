#ifndef KEYBOARD_H
#define KEYBOARD_H
#include <stdbool.h>
#include "../include/IDT.h"

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

extern volatile bool key_pressed;
extern volatile char last_keycode;

#endif
