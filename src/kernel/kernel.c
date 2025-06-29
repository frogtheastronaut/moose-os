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
    rtc_init();
    
    // Main loop with continuous time updates
    while(1) {
        // Update dock time display constantly
        dock_update_time();
        
        // Update Pong game if active
        pong_update();
        
        // Balanced delay for good responsiveness and stable Pong
        for (volatile int i = 0; i < 50000; i++) {
            // Delay loop - balanced for game speed and responsiveness
        }
    }
}