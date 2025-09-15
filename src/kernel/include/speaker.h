/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    PC Speaker driver - header file
    
    This module provides audio functionality through the PC Speaker using 
    the Programmable Interval Timer (PIT) Channel 2. The PC Speaker can 
    produce square wave tones at various frequencies.
    
    Based on: https://wiki.osdev.org/PC_Speaker
*/

#ifndef SPEAKER_H
#define SPEAKER_H

#include "../../lib/include/lib.h"
#include "pit.h"

// PC Speaker control port (Keyboard Controller port B)
#define SPEAKER_PORT         0x61

// Speaker control bits in port 0x61
#define SPEAKER_GATE_BIT     0x01  // Bit 0: Enable PIT Timer 2 to drive speaker
#define SPEAKER_DATA_BIT     0x02  // Bit 1: Direct speaker control (manual mode)

// PIT Channel 2 (PC Speaker) registers
#define PIT_SPEAKER_CHANNEL  0x42  // Channel 2 data port
#define PIT_SPEAKER_COMMAND  0x43  // Command register

// PIT Channel 2 command byte for PC Speaker
// Channel 2, Access mode: lobyte/hibyte, Mode 3 (square wave), Binary mode
#define PIT_SPEAKER_CMD      (PIT_CHANNEL_2_SEL | PIT_ACCESS_LOHI | PIT_MODE_3 | PIT_BINARY)

// Frequency constants
#define SPEAKER_BASE_FREQ    1193182  // PIT base frequency in Hz
#define SPEAKER_MIN_FREQ     20       // Minimum audible frequency
#define SPEAKER_MAX_FREQ     20000    // Maximum reasonable frequency

// Common musical note frequencies (in Hz)
#define NOTE_C4              261
#define NOTE_C_SHARP_4       277
#define NOTE_D4              294
#define NOTE_D_SHARP_4       311
#define NOTE_E4              329
#define NOTE_F4              349
#define NOTE_F_SHARP_4       370
#define NOTE_G4              392
#define NOTE_G_SHARP_4       415
#define NOTE_A4              440  // A440 - Concert pitch
#define NOTE_A_SHARP_4       466
#define NOTE_B4              494
#define NOTE_C5              523

// Beep types for convenience
#define BEEP_SHORT           100   // 100ms
#define BEEP_MEDIUM          250   // 250ms
#define BEEP_LONG            500   // 500ms

// Function declarations

/**
 * Initialize the PC Speaker system
 * Sets up the speaker but leaves it muted initially
 */
void speaker_init(void);

/**
 * Play a tone at the specified frequency
 * @param frequency Frequency in Hz (20-20000 range recommended)
 */
void speaker_play_tone(uint32_t frequency);

/**
 * Stop playing sound and mute the speaker
 */
void speaker_stop(void);

/**
 * Force complete silence (emergency stop)
 */
void speaker_force_silence(void);

/**
 * Check if the speaker is currently playing a tone
 * @return 1 if playing, 0 if muted
 */
uint8_t speaker_is_playing(void);

/**
 * Play a beep sound at the specified frequency for a duration
 * @param frequency Frequency in Hz
 * @param duration_ms Duration in milliseconds
 */
void speaker_beep(uint32_t frequency, uint32_t duration_ms);

/**
 * Play a standard system beep (1000Hz for 200ms)
 */
void speaker_system_beep(void);

/**
 * Play an error beep (low frequency, longer duration)
 */
void speaker_error_beep(void);

/**
 * Play a success beep (high frequency, short duration)
 */
void speaker_success_beep(void);

/**
 * Play a musical note by frequency
 * @param note_frequency Use NOTE_* constants or custom frequency
 * @param duration_ms Duration in milliseconds
 */
void speaker_play_note(uint32_t note_frequency, uint32_t duration_ms);

/**
 * Calculate the PIT divisor for a given frequency
 * @param frequency Desired frequency in Hz
 * @return PIT divisor value (clamped to valid range)
 */
uint16_t speaker_calculate_divisor(uint32_t frequency);

/**
 * Low-level function to set PIT Channel 2 frequency directly
 * @param divisor PIT divisor value (1-65535)
 */
void speaker_set_pit_frequency(uint16_t divisor);

/**
 * Enable the speaker gate (connect PIT Channel 2 to speaker)
 */
void speaker_enable_gate(void);

/**
 * Disable the speaker gate (disconnect PIT Channel 2 from speaker)
 */
void speaker_disable_gate(void);

/**
 * Play a startup melody
 */
void speaker_startup_melody(void);

/**
 * Play a shutdown melody
 */
void speaker_shutdown_melody(void);

/**
 * Play an attention beep (like a notification)
 */
void speaker_notification_beep(void);

/**
 * Play a warning beep pattern
 */
void speaker_warning_beep(void);

/**
 * Test function to play a scale
 */
void speaker_test_scale(void);

#endif // SPEAKER_H