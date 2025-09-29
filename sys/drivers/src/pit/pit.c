/*
    MooseOS PIT Driver
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "pit/pit.h"
#include "idt/idt.h"
#include "task/task.h"

// global timer variables
volatile uint32_t system_ticks = 0;
volatile uint32_t seconds_since_boot = 0;

static uint32_t ticks_per_second = PIT_TIMER_FREQUENCY;

/**
 * initialize the PIT with the specified frequency
 * @param frequency the desired timer frequency in Hz
 */
void pit_init(uint32_t frequency) {
    // calculate divisor
    uint32_t divisor = PIT_BASE_FREQUENCY / frequency;
    
    // ensure divisor fits in 16 bits
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    }
    
    ticks_per_second = PIT_BASE_FREQUENCY / divisor;
    
    // send command to PIT
    // channel 0, access mode: lobyte/hibyte, mode 3 (square wave), binary mode
    uint8_t command = PIT_CHANNEL_0_SEL | PIT_ACCESS_LOHI | PIT_MODE_3 | PIT_BINARY;
    outb(PIT_COMMAND, command);
    
    outb(PIT_CHANNEL_0, divisor & 0xFF); // low byte
    outb(PIT_CHANNEL_0, (divisor >> 8) & 0xFF); // high byte

    // reset tick counters
    system_ticks = 0;
    seconds_since_boot = 0;
}

/**
 * set a new frequency for the PIT
 * @param frequency the new frequency in Hz
 */
void pit_set_frequency(uint32_t frequency) {
    // reinitialize PIT with new frequency
    pit_init(frequency);
}

/**
 * @return number of timer ticks since boot
 */
uint32_t pit_get_ticks(void) {
    return system_ticks;
}

/**
 * @return seconds since boot
 */
uint32_t pit_get_seconds(void) {
    return seconds_since_boot;
}

/**
 * timer interrupt handler
 */
void timer_interrupt_handler(void) {
    system_ticks++;
    
    if (system_ticks % ticks_per_second == 0) {
        seconds_since_boot++;
    }
    
    task_tick();
    kernel_update_time();
    
    // signal end of interrupt to PIC
    outb(0x20, 0x20);
}