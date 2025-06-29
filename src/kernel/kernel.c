/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/
#include "include/tty.h"
#include "include/IDT.h"
#include "../gui/include/explorer.h"
#include "../gui/include/gui.h"
#include "../gui/include/dock.h"
#include "../gui/include/pong.h"
#include "../time/rtc.h"
/*

simple code to run a simple kernel. 
 - MooseOS guy
*/
void kernel_main(void) 
{
    // INIT DA TERMINALLL
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
    // sample file
    filesys_mkfile("HELP.txt", "Hello! Welcome to MooseOS!\n"
                           "Controls:\n"
                           "- Use arrow keys to navigate\n"
                            "- Press Enter to open selection\n"
                           "- Press Escape to exit\n"
                           "- 'D' to make folder, 'F' to make file.\n"
                           "You are running MooseOS version 0.1.0. \nEnjoy!\n\n"
                           "Copyright 2025 Ethan Zhang\n");
    dock_init();
    rtc_init();
    
    // loopity loopity
    while(1) {
        // update dock time
        dock_update_time();
        
        // update pong if active
        pong_update();
        
        // big lag prevention loop
        for (volatile int i = 0; i < 50000; i++) {
            // nothing here but safety
        }
    }
}