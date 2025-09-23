/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#ifndef SPEAKER_H
#define SPEAKER_H

#include "libc/lib.h"
#include "pit/pit.h"

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
// Concert pitch for those of you who don't understand music
#define NOTE_C4              261
#define NOTE_C_SHARP_4       277
#define NOTE_D4              294
#define NOTE_D_SHARP_4       311
#define NOTE_E4              329
#define NOTE_F4              349
#define NOTE_F_SHARP_4       370
#define NOTE_G4              392
#define NOTE_G_SHARP_4       415
#define NOTE_A4              440
#define NOTE_A_SHARP_4       466
#define NOTE_B4              494
#define NOTE_C5              523

// Beep types for convenience
#define BEEP_SHORT           100   // 100ms
#define BEEP_MEDIUM          250   // 250ms
#define BEEP_LONG            500   // 500ms

// Function declarations
void speaker_init(void);
void speaker_play_tone(uint32_t frequency);
void speaker_stop(void);
void speaker_force_silence(void);
uint8_t speaker_is_playing(void);
void speaker_beep(uint32_t frequency, uint32_t duration_ms);
void speaker_system_beep(void);
void speaker_error_beep(void);
void speaker_success_beep(void);
void speaker_play_note(uint32_t note_frequency, uint32_t duration_ms);
uint16_t speaker_calculate_divisor(uint32_t frequency);
void speaker_set_pit_frequency(uint16_t divisor);
void speaker_enable_gate(void);
void speaker_disable_gate(void);
void speaker_startup_melody(void);
void speaker_shutdown_melody(void);
void speaker_notification_beep(void);
void speaker_warning_beep(void);
void speaker_test_scale(void);

#endif // SPEAKER_H