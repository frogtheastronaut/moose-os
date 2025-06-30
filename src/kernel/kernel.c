/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/
#include "include/tty.h"
#include "include/IDT.h"
#include "include/task.h"
#include "../gui/include/explorer.h"
#include "../gui/include/gui.h"
#include "../gui/include/dock.h"
#include "../gui/include/pong.h"
#include "../time/rtc.h"
/*

simple code to run a simple kernel. 
 - MooseOS guy
*/

void init_filesys() {
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
}

void dock() {
    while (1) {
        if (dock_is_active()) {
            dock_update_time();
            for (volatile int i = 0; i < 50000; i++) {} // Normal UI delay
        } else if (pong_active) {
            pong_update();
            for (volatile int i = 0; i < 400000; i++) {} // Slower Pong delay
        } else {
            for (volatile int i = 0; i < 50000; i++) {} // Fallback
        }
        task_yield();
    }
}

void kernel_main(void) 
{
    // INNIT
    gui_init();
    idt_init();
    kb_init();
    dock_init();
    rtc_init();
    init_filesys();
    task_init();

    // make dock
    task_create(dock);

    // loopity loopity
    while(1) {
        task_schedule();
    }
}