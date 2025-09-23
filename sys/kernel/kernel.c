/*
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/
/*
simple code to run a simple OS
 - MooseOS guy 
 (Do not remove)
*/

#include "idt/IDT.h"
#include "paging/paging.h"
#include "task/task.h"
#include "mouse/mouse.h"
#include "keyboard/keyboard.h"
#include "ata/ata.h"
#include "libc/lib.h"
#include "explorer.h"
#include "gui/gui.h"
#include "dock.h"
#include "rtc/rtc.h"
#include "pit/pit.h"
#include "vga/vga.h"
#include "speaker/speaker.h"
#include "stdio/qstdio.h"
#include "qemu/qemu.h"
#include "isr/isr.h"

extern bool explorer_active;
extern volatile uint32_t ticks;
extern volatile bool key_pressed;
extern unsigned char last_keycode;
extern unsigned char keyboard_map_normal[128];

void init_filesys() {
    // Initialize disk subsystem first
    disk_init();
    
    // Initialize in-memory filesystem structure
    filesystem_init();
    
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
    filesystem_make_dir("Documents");
    filesystem_make_dir("Desktop");
    filesystem_make_dir("Apps");
    filesystem_make_dir("Photos");
    filesystem_make_dir("Library");
    // sample file
    filesystem_make_file("HELP.txt", "Hello! Welcome to MooseOS!\n"
                        "Controls:\n"
                        "- Use arrow keys to navigate\n"
                            "- Press Enter to open selection\n"
                        "- Press Escape to exit\n"
                        "- 'D' to make folder, 'F' to make file.\n"
                        "You are running MooseOS version 0.2 \nEnjoy!\n\n"
                        "Copyright 2025 Ethan Zhang\n");
    
    // Format the disk and save the default filesystem
    int format_result = filesystem_format(0);
    if (format_result == 0) {
        // Format succeeded, now save memory to disk
        filesystem_save_to_disk();
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
    dock_update_time();
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
    // Debugf only prints in QEMU environment.
    debugf("[MOOSE]: QEMU Environment Detected\n");
    /** 
     * We would now initialise every. Single. Thing.
     * @note: initialisation order is important.
     */
    vga_init_custom_palette();

    gui_init();
    debugf("[MOOSE]: GUI initialised\n");

    // Initialize paging with 16MB of memory 
    paging_init(16 * 1024 * 1024);
    debugf("[MOOSE]: Paging initialised\n");
    // Must initialise IDT after paging because IDT also initialises GDT.
    idt_init();
    debugf("[MOOSE]: IDT initialised\n");
    isr_init();
    debugf("[MOOSE]: ISR handlers installed\n");

    mouse_init(); 
    debugf("[MOOSE]: Mouse initialised\n");

    keyboard_init(); 
    debugf("[MOOSE]: Keyboard initialised\n");

    dock_init();
    debugf("[MOOSE]: Dock initialised\n");
    
    // Initialize RTC for initial time sync only
    rtc_init();
    debugf("[MOOSE]: RTC initialised\n");
    
    // Initialize PIT for system timing (1000 Hz = 1ms intervals)
    pit_init(PIT_TIMER_FREQUENCY);
    debugf("[MOOSE]: PIT initialised\n");

    // Initialize PC Speaker for audio output
    speaker_init();
    debugf("[MOOSE]: PC Speaker initialised\n");
    
    init_filesys();
    debugf("[MOOSE]: Filesystem initialised\n");
    task_init();
    
    debugf("[MOOSE]: Multitasking initialised\n");

    // Register tasks in our simple scheduler
    register_task(kernel_handle_interrupts);
    task_create(main_loop);
    
    // Test basic speaker functionality with a simple beep
    speaker_startup_melody();
    
    
    // Start the task system
    task_start();
    
}