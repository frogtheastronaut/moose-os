/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/
#include "include/tty.h"
#include "include/IDT.h"
#include "include/task.h"
#include "include/mouse.h"
#include "../lib/lib.h"
#include "../gui/include/explorer.h"
#include "../gui/include/gui.h"
#include "../gui/include/dock.h"
#include "../time/rtc.h"
/*

simple code to run a simple kernel. 
 - MooseOS guy
*/

extern bool explorer_active;

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
    // Initialize mouse cursor on first run
    static bool first_run = true;
    static uint32_t last_cursor_update = 0;
    if (first_run) {
        gui_update_mouse_cursor(); // Force initial mouse cursor draw
        first_run = false;
    }
    
    while (1) {
        // Handle mouse input based on current active application
        if (dock_is_active()) {
            // Handle mouse for dock when dock is active
            dock_handle_mouse();
        } else if (explorer_active) {
            // Handle mouse for file explorer when explorer is active
            explorer_handle_mouse();
        }
        // Note: Mouse input is disabled for editor and terminal
        
        // Throttle mouse cursor updates to prevent trails - only update every few ticks
        extern volatile uint32_t ticks;
        if (ticks - last_cursor_update >= 2) { // Update every 20ms (more responsive)
            gui_update_mouse_cursor();
            last_cursor_update = ticks;
        }
        
        // SIMPLE: Just handle time updates without complex conditions
        // This prevents any potential deadlock scenarios
        dock_update_time();
        
        // Always yield to maintain responsive multitasking
        task_yield();
    }
}; 

void kernel_main(void) 
{
    // INNIT
    gui_init();
    idt_init();
    kb_init();
    mouse_init();  // Initialize mouse support
    // Set up PIT for timer interrupts (100Hz - original frequency)
    outb(0x43, 0x36); // PIT command: channel 0, lo/hi byte, mode 3
    outb(0x40, 11932 & 0xFF); // low byte (1193182 / 100)
    outb(0x40, 11932 >> 8);   // high byte
    dock_init();
    rtc_init();
    init_filesys();
    task_init();

    // make dock
    task_create(dock);

    task_start();
    while(1) {}
}