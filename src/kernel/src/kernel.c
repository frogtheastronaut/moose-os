/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

/*
    ================================ OS THEORY ================================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.
    
    The kernel is the CORE of any operating system. When your computer boots up, the kernel is the first program
    that runs and it never stops running until you shut down. This file is very important!
    
    WHAT DOES THE KERNEL DO?
    1. HARDWARE MANAGEMENT: Controls CPU, memory, disk, keyboard, mouse, graphics
    2. PROCESS MANAGEMENT: Runs multiple programs at the same time (multitasking)
    3. MEMORY MANAGEMENT: Decides which programs get which parts of memory
    4. SECURITY: Prevents programs from interfering with each other
    5. RESOURCE ALLOCATION: Shares CPU time, memory, and hardware between programs

    Source: https://wiki.osdev.org/Kernel
*/

/*
simple code to run a simple OS
 - MooseOS guy 
 (Do not remove)
*/

// #include "include/tty.h"
#include "../include/IDT.h"
#include "../include/paging.h"
#include "../include/task.h"
#include "../include/mouse.h"
#include "../include/disk.h"
#include "../../lib/include/lib.h"
#include "../../gui/include/explorer.h"
#include "../../gui/include/gui.h"
#include "../../gui/include/dock.h"
#include "../../time/include/rtc.h"
#include "../include/pit.h"
#include "../include/vga.h"


extern bool explorer_active;
extern volatile uint32_t ticks;

void init_filesys() {
    // Initialize disk subsystem first
    disk_init();
    
    // Initialize in-memory filesystem structure
    fs_init();
    
    // Try to mount existing filesystem from disk
    int mount_result = fs_mount(0);
    if (mount_result == 0) {
        // Mount succeeded, try to load existing filesystem data
        int load_result = fs_load_from_disk();
        if (load_result == 0) {
            // Successfully loaded existing filesystem from disk
            return;
        }
        // If load failed, fall through to create default filesystem
    }
    
    // No valid filesystem found or load failed, create default filesystem
    fs_make_dir("Documents");
    fs_make_dir("Desktop");
    fs_make_dir("Apps");
    fs_make_dir("Photos");
    fs_make_dir("Library");
    // sample file
    fs_make_file("HELP.txt", "Hello! Welcome to MooseOS!\n"
                        "Controls:\n"
                        "- Use arrow keys to navigate\n"
                            "- Press Enter to open selection\n"
                        "- Press Escape to exit\n"
                        "- 'D' to make folder, 'F' to make file.\n"
                        "You are running MooseOS version 0.5BETA. \nEnjoy!\n\n"
                        "Copyright 2025 Ethan Zhang\n");
    
    // Format the disk and save the default filesystem
    int format_result = fs_format(0);
    if (format_result == 0) {
        // Format succeeded, now save the current in-memory filesystem to disk
        fs_save_to_disk();
    }
}

void kernel_handle_interrupts() {
    static uint32_t last_cursor_update = 0;
    if (key_pressed) {
        processKey(keyboard_map_normal[(unsigned char)last_keycode], last_keycode);
        key_pressed = false;
    }
    
    if (ticks - last_cursor_update >= 2) {
        gui_updatemouse();
        last_cursor_update = ticks;
    }
    if (dock_is_active()) {
        dock_handle_mouse();
    } else if (explorer_active) {
        explorer_handle_mouse();
    }
}

void kernel_update_time() {
    dock_update_time();
}


// Main kernel loop with simple cooperative scheduler
void main_loop() {
    while (1) {
        run_tasks();
        task_yield();
    }
}

void kernel_main(void) 
{
    vga_init_custom_palette();
    // INNIT
    gui_init();
    idt_init();
    
    // Initialize paging with 16MB of memory (adjust as needed)
    paging_init(16 * 1024 * 1024);

    
    mouse_init(); 

    kb_init(); 
    
    dock_init();
    
    // Initialize RTC for initial time sync only
    rtc_init();
    
    // Initialize PIT for system timing (1000 Hz = 1ms intervals)
    pit_init(PIT_TIMER_FREQUENCY);
    init_filesys();
    task_init();

    // Register tasks in our simple scheduler
    register_task(kernel_handle_interrupts);
    register_task(kernel_update_time);
    task_create(main_loop);
    
    // Start the task system
    task_start();
}