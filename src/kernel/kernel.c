/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang.
*/

/*

simple code to run a simple OS
 - MooseOS guy 
 (Do not remove)
*/

// #include "include/tty.h"
#include "include/IDT.h"
#include "include/paging.h"
#include "include/task.h"
#include "include/mouse.h"
#include "include/disk.h"
#include "../lib/lib.h"
#include "../gui/include/explorer.h"
#include "../gui/include/gui.h"
#include "../gui/include/dock.h"
#include "../time/rtc.h"


extern bool explorer_active;
extern volatile uint32_t ticks;

void init_filesys() {
    // Initialize disk subsystem first
    disk_init();
    
    // Initialize in-memory filesystem structure
    filesys_init();
    
    // Try to mount existing filesystem from disk
    int mount_result = filesys_mount(0);
    if (mount_result == 0) {
        // Mount succeeded, try to load existing filesystem data
        int load_result = filesys_load_from_disk();
        if (load_result == 0) {
            // Successfully loaded existing filesystem from disk
            return;
        }
        // If load failed, fall through to create default filesystem
    }
    
    // No valid filesystem found or load failed, create default filesystem
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
                        "You are running MooseOS version 0.5BETA. \nEnjoy!\n\n"
                        "Copyright 2025 Ethan Zhang\n");
    
    // Format the disk and save the default filesystem
    int format_result = filesys_format(0);
    if (format_result == 0) {
        // Format succeeded, now save the current in-memory filesystem to disk
        filesys_save_to_disk();
    }
}

void dock() {
    //static bool first_run = true;
    static uint32_t last_cursor_update = 0;
    // if (first_run) {
    //     gui_updatemouse(); 
    //     first_run = false;
    // }
    
    while (1) {
        if (dock_is_active()) {
            dock_handle_mouse();
        } else if (explorer_active) {
            explorer_handle_mouse();
        }

        // Keyboard event processing
        if (key_pressed) {
            processKey(keyboard_map_normal[(unsigned char)last_keycode], last_keycode);
            key_pressed = false;
        }

        if (ticks - last_cursor_update >= 2) {
            gui_updatemouse();
            last_cursor_update = ticks;
        }
        dock_update_time();
        task_yield();
    }
}; 

void kernel_main(void) 
{
    // INNIT
    gui_init();
    idt_init();
    
    // Initialize paging with 16MB of memory (adjust as needed)
    paging_init(16 * 1024 * 1024);
    
    kb_init();
    mouse_init(); 

    // yeah idk either. this just happens to work
    outb(0x43, 0x36); 
    outb(0x40, 11932 & 0xFF); 
    outb(0x40, 11932 >> 8);   

    dock_init();
    rtc_init();
    init_filesys();
    task_init();

    // make dock
    task_create(dock);
    task_start();
    while(1) {} // ...
}