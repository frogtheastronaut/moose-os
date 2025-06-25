/*
	SimpleM
	Copyright 2025 Ethan Zhang
*/
#include "include/tty.h"
#include "include/IDT.h"
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
    filesys_mkdir("hello");
     filesys_mkdir("hello1");
      filesys_mkdir("hello2");
       filesys_mkdir("hello3");
        filesys_mkdir("hello4");
    filesys_mkfile("hello.txt", "bonjour");
    filesys_mkfile("hello2.txt", "bonjour2");
    
    //shell_prompt(); // Show initial shell prompt
    
    // Show GUI demo with key-based exit
    gui_draw_filesplorer();
    
    // After exiting GUI, your terminal will be restored
    //shell_prompt();
    
    while(1);
}