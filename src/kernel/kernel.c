/*
	SimpleM
	Copyright 2025 Ethan Zhang
*/

#include "include/tty.h"
#include "include/IDT.h"

/*

Simple code to run a simple kernel. 

*/
void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
	//terminal_writestring("Welcome to the SimpleM kernel", true);
	idt_init();
	kb_init();
	while(1);
}