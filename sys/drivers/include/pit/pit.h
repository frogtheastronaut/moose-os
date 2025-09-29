/*
    MooseOS Programmable Interval Timer
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
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
#define PIT_BINARY       0x00  // use Binary mode
#define PIT_BCD          0x01  // use BCD mode

#define PIT_MODE_0       0x00  // interrupt on terminal count
#define PIT_MODE_1       0x02  // hardware re-triggerable one-shot
#define PIT_MODE_2       0x04  // rate generator
#define PIT_MODE_3       0x06  // square wave generator
#define PIT_MODE_4       0x08  // software triggered strobe
#define PIT_MODE_5       0x0A  // hardware triggered strobe

#define PIT_ACCESS_LATCH 0x00  // latch count value command
#define PIT_ACCESS_LO    0x10  // access low byte only
#define PIT_ACCESS_HI    0x20  // access high byte only
#define PIT_ACCESS_LOHI  0x30  // access low byte then high byte

#define PIT_CHANNEL_0_SEL 0x00
#define PIT_CHANNEL_1_SEL 0x40
#define PIT_CHANNEL_2_SEL 0x80

// PIT frequency constants
#define PIT_BASE_FREQUENCY   1193182  // 1.193182 MHz base frequency
#define PIT_TIMER_FREQUENCY  1000     // 1000 Hz (1ms intervals)
#define PIT_TIMER_DIVISOR    (PIT_BASE_FREQUENCY / PIT_TIMER_FREQUENCY)

// timer interrupt vector (IRQ0)
#define TIMER_IRQ            0

// global timer variables
extern volatile uint32_t system_ticks;
extern volatile uint32_t seconds_since_boot;

// external function declarations
extern void kernel_update_time(void);
extern void task_tick(void);

// function declarations
void pit_init(uint32_t frequency);
void pit_set_frequency(uint32_t frequency);
uint32_t pit_get_ticks(void);
uint32_t pit_get_seconds(void);
void timer_interrupt_handler(void);

#endif // PIT_H