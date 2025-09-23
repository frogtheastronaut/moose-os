/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    .h file for ./pit.c - Programmable Interval Timer
*/
#ifndef PIT_H
#define PIT_H

#include "libc/lib.h"

// PIT I/O port addresses
#define PIT_CHANNEL_0    0x40
#define PIT_CHANNEL_1    0x41  
#define PIT_CHANNEL_2    0x42
#define PIT_COMMAND      0x43

// PIT command register bits
#define PIT_BINARY       0x00  // Use Binary mode
#define PIT_BCD          0x01  // Use BCD mode

#define PIT_MODE_0       0x00  // Interrupt on Terminal Count
#define PIT_MODE_1       0x02  // Hardware Re-triggerable One-shot
#define PIT_MODE_2       0x04  // Rate Generator
#define PIT_MODE_3       0x06  // Square Wave Generator
#define PIT_MODE_4       0x08  // Software Triggered Strobe
#define PIT_MODE_5       0x0A  // Hardware Triggered Strobe

#define PIT_ACCESS_LATCH 0x00  // Latch count value command
#define PIT_ACCESS_LO    0x10  // Access low byte only
#define PIT_ACCESS_HI    0x20  // Access high byte only
#define PIT_ACCESS_LOHI  0x30  // Access low byte then high byte

#define PIT_CHANNEL_0_SEL 0x00
#define PIT_CHANNEL_1_SEL 0x40
#define PIT_CHANNEL_2_SEL 0x80

// PIT frequency constants
#define PIT_BASE_FREQUENCY   1193182  // 1.193182 MHz base frequency
#define PIT_TIMER_FREQUENCY  1000     // 1000 Hz (1ms intervals)
#define PIT_TIMER_DIVISOR    (PIT_BASE_FREQUENCY / PIT_TIMER_FREQUENCY)

// Timer interrupt vector (IRQ0)
#define TIMER_IRQ            0

// Global timer variables
extern volatile uint32_t system_ticks;
extern volatile uint32_t seconds_since_boot;

// External function declarations
extern void kernel_update_time(void);
extern void task_tick(void);

// Function declarations
void pit_init(uint32_t frequency);
void pit_set_frequency(uint32_t frequency);
uint32_t pit_get_ticks(void);
uint32_t pit_get_seconds(void);
void pit_wait(uint32_t milliseconds);
void timer_interrupt_handler(void);

#endif