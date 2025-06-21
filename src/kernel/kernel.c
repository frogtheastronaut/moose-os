/*
	SimpleM
	Copyright 2025 Ethan Zhang
*/
#include "include/tty.h"
#include "include/IDT.h"
#include "../shell/shell.h"
#include "../gui/gui.h"
/*

Simple code to run a simple kernel. 

*/
void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
	terminal_writestring("Welcome to the SimpleM kernel", true);
	idt_init();
	kb_init();
    filesys_init();
    //demo(); // from file.c
	shell_prompt(); // from shell.c
	test_draw_rect();
	while(1);

}