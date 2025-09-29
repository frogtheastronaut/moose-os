/*
    MooseOS Panic code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "panic/panic.h"
#include "vga/vga.h"
#include "gui/gui.h"
#include "print/debug.h"

void panic(const char* message) {
	asm volatile("cli");
	
	gui_clear(VGA_COLOUR_BLUE);
	
	draw_text(10, 50, "KERNEL PANIC", VGA_COLOUR_WHITE);
	draw_text(10, 70, message, VGA_COLOUR_WHITE);
	draw_text(10, 100, "System halted. Please restart.", VGA_COLOUR_WHITE);
	debugf("[MOOSE] PANIC!\n");
	
	// halt the CPU
	while (1) {
		asm volatile("hlt");
	}
}