#include "panic/panic.h"
#include "vga/vga.h"
#include "gui/gui.h"
#include "print/debug.h"

void panic(const char* message) {
	asm volatile("cli");
	
	gui_clear(VGA_COLOR_BLUE);
	
	draw_text(10, 50, "KERNEL PANIC", VGA_COLOR_WHITE);
	draw_text(10, 70, message, VGA_COLOR_WHITE);
	draw_text(10, 100, "System halted. Please restart.", VGA_COLOR_WHITE);
	debugf("[MOOSE] PANIC!");
	
	// Halt the CPU indefinitely
	while (1) {
		asm volatile("hlt");
	}
}