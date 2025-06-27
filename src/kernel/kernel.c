/*
	SimpleM
	Copyright 2025 Ethan Zhang
*/
#include "include/tty.h"
#include "include/IDT.h"
#include "../gui/explorer.h"
#include "../gui/gui.h"
#include "../gui/dock.h"
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
    filesys_mkdir("Documents");
    filesys_mkdir("Desktop");
    filesys_mkdir("Apps");
    filesys_mkdir("Photos");
    filesys_mkdir("Library");
    filesys_mkfile("HELP.txt", "Hello! Welcome to MooseOS!\n"
                           "Controls:\n"
                           "- Use arrow keys to navigate\n"
                            "- Press Enter to open selection\n"
                           "- Press Escape to exit\n"
                           "- 'D' to make folder, 'F' to make file.\n"
                           "You are running MooseOS version 0.1.0. \nEnjoy!\n\n"
                           "Copyright 2025 Ethan Zhang\n");
    dock_init();
    while(1);
}