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
    terminal_writestring("Welcome to the MooseOS kernel", true);
    idt_init();
    kb_init();
    filesys_init();
    
    shell_prompt(); // Show initial shell prompt
    
    // Show GUI demo with key-based exit
    gui_demo();
    
    // After exiting GUI, your terminal will be restored
    shell_prompt();
    
    while(1);
}