/*
    MooseOS Kernel
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "idt/idt.h"
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
#include "print/debug.h"
#include "qemu/qemu.h"
#include "isr/test_interrupts.h"
#include "isr/isr.h"
#include "test_interrupts.h"

// external variables
extern bool explorer_active;
extern volatile uint32_t ticks;
extern volatile bool key_pressed;
extern unsigned char last_keycode;
extern unsigned char keyboard_map_normal[128];

void init_filesys() {
    // we will proceed to intialise everything
    disk_init();
    filesystem_init();
    
    // try to mount existing filesystem from disk
    int mount_result = filesystem_mount(0);
    if (mount_result == 0) {
        // mount succeeded, try to load existing filesystem data
        int load_result = filesystem_load_from_disk();
        if (load_result == 0) {
            // successfully loaded existing filesystem from disk
            return;
        }
        // if load failed, fall through to create default filesystem
        debugf("[MOOSE] Failed to load filesystem from disk, creating default filesystem\n");
    }
    /**
     * @note this should show empty directory as we are initialising an empty filessystem
     */
    filesystem_make_dir("Documents");
    filesystem_make_dir("Desktop");
    filesystem_make_dir("Apps");
    filesystem_make_dir("Photos");
    filesystem_make_dir("Library");
    filesystem_make_file("HELP.txt", "Hello! Welcome to MooseOS!\n"
                        "Controls:\n"
                        "- Use arrow keys to navigate\n"
                            "- Press Enter to open selection\n"
                        "- Press Escape to exit\n"
                        "- 'D' to make folder, 'F' to make file.\n"
                        "You are running MooseOS version 0.2 \nEnjoy!\n\n"
                        "Copyright 2025 Ethan Zhang\n");
    
    // format the disk and save the default filesystem
    int format_result = filesystem_format(0);
    if (format_result == 0) {
        // format succeeded, now save memory to disk
        filesystem_save_to_disk();
    } else {
        // failed
        debugf("[MOOSE] Failed to format filesystem on disk\n");
    }
}

// handle interrupt task
void kernel_handle_interrupts() {
    static uint32_t last_cursor_update = 0;
    if (key_pressed) {
        process_key(keyboard_map_normal[(unsigned char)last_keycode], last_keycode);
        key_pressed = false;
    }
    
    if (ticks - last_cursor_update >= 2) {
        gui_update_mouse();
        last_cursor_update = ticks;
    }
    if (dock_is_active()) {
        dock_handle_mouse();
    } else if (explorer_active) {
        explorer_handle_mouse();
    }
    dock_update_time();
}

// update time task
void kernel_update_time() {
    dock_update_time();
}


// main kernel loop
void main_loop() {
    while (1) {
        run_tasks();
        task_yield();
    }
}

void kernel_main(void) 
{
    /**
     * @note debugf only prints in QEMU environment.
     */
    debugf("[MOOSE]: QEMU Environment Detected\n");
    /** 
     * we would now initialise every. single. thing.
     * @note: initialisation order is important.
     */
    vga_init_custom_palette();

    gui_init();
    debugf("[MOOSE]: GUI initialised\n");

    // initialize paging with 16MB of memory
    paging_init(16 * 1024 * 1024);
    debugf("[MOOSE]: Paging initialised\n");
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
    
    // initialise RTC to sync the time
    rtc_init();
    debugf("[MOOSE]: RTC initialised\n");
    
    pit_init(PIT_TIMER_FREQUENCY);
    debugf("[MOOSE]: PIT initialised\n");

    speaker_init();
    debugf("[MOOSE]: PC Speaker initialised\n");
    
    init_filesys();
    debugf("[MOOSE]: Filesystem initialised\n");
    task_init();
    
    debugf("[MOOSE]: Multitasking initialised\n");

    // register interrupts
    register_task(kernel_handle_interrupts);

    // create main loop task
    task_create(main_loop);
    
    // play the startup melody
    speaker_startup_melody();
    
    // start the task system
    task_start();
}