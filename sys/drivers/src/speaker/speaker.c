/*
    MooseOS PC Speaker Driver
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "speaker/speaker.h"
#include "io/io.h"

// internal state tracking variables
static uint8_t speaker_initialized = 0;
static uint8_t speaker_playing = 0;
static uint8_t original_speaker_state = 0;

/**
 * Initialise the PC Speaker system
 */
void speaker_init(void) {
    if (speaker_initialized) {
        return; // already initialised
    }
    
    original_speaker_state = inb(SPEAKER_PORT);
    
    // mute the speaker
    uint8_t current_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current_state & ~(SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    
    // configure PIT Channel 2 for square wave mode
    outb(PIT_COMMAND, PIT_SPEAKER_CMD);

    // set the variables
    speaker_playing = 0;
    speaker_initialized = 1;
}

/**
 * calculate the PIT divisor for a given frequency
 * @param frequency desired frequency in Hz
 * @return PIT divisor value (clamped to valid range)
 */
uint16_t speaker_calculate_divisor(uint32_t frequency) {
    if (frequency < SPEAKER_MIN_FREQ) {
        frequency = SPEAKER_MIN_FREQ;
    } else if (frequency > SPEAKER_MAX_FREQ) {
        frequency = SPEAKER_MAX_FREQ;
    }
    
    uint32_t divisor = SPEAKER_BASE_FREQ / frequency;
    
    // clamp to 16-bit range
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    } else if (divisor < 1) {
        divisor = 1;
    }
    
    return (uint16_t)divisor;
}

/**
 * set PIT Channel 2 frequency
 * @param divisor PIT divisor value
 */
void speaker_set_pit_frequency(uint16_t divisor) {
    outb(PIT_SPEAKER_CHANNEL, divisor & 0xFF); // Low byte
    outb(PIT_SPEAKER_CHANNEL, (divisor >> 8) & 0xFF); // High byte
}

/**
 * enable speaker gate
 */
void speaker_enable_gate(void) {
    uint8_t current_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current_state | (SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    speaker_playing = 1;
}

/**
 * disable speaker gate
 */
void speaker_disable_gate(void) {
    uint8_t current_state = inb(SPEAKER_PORT);
    // Clear gate bit (bit 0)
    outb(SPEAKER_PORT, current_state & ~SPEAKER_GATE_BIT);
    speaker_playing = 0;
}

/**
 * play a tone at the specified frequency
 * @param frequency frequency in Hz
 */
void speaker_play_tone(uint32_t frequency) {
    if (!speaker_initialized) {
        speaker_init();
    }
    
    uint16_t divisor = speaker_calculate_divisor(frequency);
    
    outb(PIT_COMMAND, PIT_SPEAKER_CMD);
    speaker_set_pit_frequency(divisor);
    
    // enable the speaker
    speaker_enable_gate();
}

/**
 * stop playing sound and mute the speaker
 */
void speaker_stop(void) {
    // disconnect PIT from speaker
    speaker_disable_gate();

    // clear both bits
    uint8_t current_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current_state & ~(SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    
    speaker_playing = 0;
}

/**
 * check if the speaker is currently playing
 * @return 1 if playing, 0 if muted
 */
uint8_t speaker_is_playing(void) {
    return speaker_playing;
}

/**
 * Delay speaker
 * @param milliseconds approximate milliseconds to wait
 * 
 * @todo there's got to be a better way to delay this speaker
 */
static void speaker_delay(uint32_t milliseconds) {
    volatile uint32_t cycles = milliseconds * 100000;
    for (volatile uint32_t i = 0; i < cycles; i++) {
        asm volatile("nop");
    }
}

/**
 * Play a beep sound at the specified frequency for a duration
 * @param frequency Frequency in Hz
 * @param duration_ms Duration in milliseconds
 */
void speaker_beep(uint32_t frequency, uint32_t duration_ms) {
    speaker_play_tone(frequency);
    speaker_delay(duration_ms);
    speaker_stop();
}

/**
 * the following code plays various beeps.
 * BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP
 * BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP BEEP
 * 
 * @todo ADD MORE BEEPS
 * get it to play a tune?
 */
void speaker_system_beep(void) {
    speaker_beep(1000, 200);
}
void speaker_error_beep(void) {
    speaker_beep(200, 150);
    speaker_delay(50);
    speaker_beep(200, 150);
    speaker_delay(50);
    speaker_beep(200, 300);
}
void speaker_success_beep(void) {
    speaker_beep(1500, 100);
    speaker_delay(50);
    speaker_beep(2000, 150);
}

void speaker_play_note(uint32_t note_frequency, uint32_t duration_ms) {
    speaker_beep(note_frequency, duration_ms);
}
void speaker_startup_melody(void) {
    speaker_play_note(NOTE_A4, 200);
}
void speaker_notification_beep(void) {
    speaker_beep(800, 100);
    speaker_delay(100);
    speaker_beep(1200, 150);
}
void speaker_warning_beep(void) {
    for (int i = 0; i < 5; i++) {
        speaker_beep(400, 80);
        speaker_delay(80);
    }
}
void speaker_test_scale(void) {
    uint32_t notes[] = {
        NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, 
        NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
    };
    
    const char* note_names[] = {
        "C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5"
    };
    
    for (int i = 0; i < 8; i++) {
        speaker_play_note(notes[i], 300);
        speaker_delay(100);
    }
}
