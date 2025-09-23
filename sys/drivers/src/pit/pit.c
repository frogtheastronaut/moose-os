/*
    MooseOS 
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

/*    
    ====================== OS THEORY ======================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.

    WHAT IS THE PIT (Programmable Interval Timer)?
    The PIT is a hardware timer chip (Intel 8253/8254) that can generate periodic interrupts.
    It's commonly used for system timing, task scheduling, and providing a steady clock source.
    
    The PIT has three channels:
    - Channel 0: Usually connected to IRQ0 for system timer
    - Channel 1: Used for DRAM refresh (legacy)
    - Channel 2: Connected to PC speaker
    
    For our OS, we'll use Channel 0 to generate timer interrupts at regular intervals.
    This is much more efficient than constantly polling the RTC for time updates.
    
    The PIT operates at a base frequency of 1.193182 MHz. To get different frequencies,
    we divide this base frequency by a divisor value.
*/

#include "pit/pit.h"
#include "idt/IDT.h"
#include "task/task.h"

// Global timer variables
volatile uint32_t system_ticks = 0;
volatile uint32_t seconds_since_boot = 0;

// Ticks per second (should match PIT_TIMER_FREQUENCY)
static uint32_t ticks_per_second = PIT_TIMER_FREQUENCY;

/**
 * Initialize the PIT with the specified frequency
 * @param frequency The desired timer frequency in Hz
 */
void pit_init(uint32_t frequency) {
    // Calculate the divisor for the desired frequency
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    
    // Ensure divisor fits in 16 bits
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    }
    
    // Store the actual frequency we're using
    ticks_per_second = PIT_BASE_FREQUENCY / divisor;
    
    // Send command to PIT
    // Channel 0, Access mode: lobyte/hibyte, Mode 3 (square wave), Binary mode
    uint8_t command = PIT_CHANNEL_0_SEL | PIT_ACCESS_LOHI | PIT_MODE_3 | PIT_BINARY;
    outb(PIT_COMMAND, command);
    
    // Send the divisor (low byte first, then high byte)
    outb(PIT_CHANNEL_0, divisor & 0xFF);
    outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF);
    
    // Reset tick counters
    system_ticks = 0;
    seconds_since_boot = 0;
}

/**
 * Set a new frequency for the PIT
 * @param frequency The new frequency in Hz
 */
void pit_set_frequency(uint32_t frequency) {
    pit_init(frequency);
}

/**
 * Get the current system tick count
 * @return Number of timer ticks since boot
 */
uint32_t pit_get_ticks(void) {
    return system_ticks;
}

/**
 * Get the number of seconds since boot
 * @return Seconds since boot
 */
uint32_t pit_get_seconds(void) {
    return seconds_since_boot;
}

/**
 * Timer interrupt handler - called on every PIT interrupt
 * This function is called from the assembly interrupt handler
 */
void timer_interrupt_handler(void) {
    // Increment tick counter
    system_ticks++;
    
    // Update seconds counter
    if (system_ticks % ticks_per_second == 0) {
        seconds_since_boot++;
    }
    
    // Call the task system's tick handler
    task_tick();
    
    // Call the kernel's time update function
    // This maintains compatibility with existing code
    kernel_update_time();
    
    // Signal end of interrupt to PIC
    outb(0x20, 0x20);
}