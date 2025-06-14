#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include <stddef.h>
#include "tty.c"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* data, size_t size);
void terminal_writestring(const char* data);
void terminal_scroll(int line);
void terminal_delete_last_line();

#endif