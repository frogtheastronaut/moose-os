/*
    MooseOS 
    Copyright (c) 2025 Ethan Zhang and Contributors.
    
    PC Speaker driver implementation
    
    ====================== OS THEORY ======================
    The PC Speaker is a basic audio output device found on all PC-compatible systems.
    It works by rapidly moving a speaker cone "in" and "out" to create sound waves.
    
    The speaker has only two positions: "in" (0) and "out" (1). By changing between
    these positions at different frequencies, we can create audible tones.
    
    There are two main ways to control the PC Speaker:
    1. Direct control via port 0x61 bit 1 (manual toggle)
    2. Automatic control via PIT Channel 2 (what we use here)
    
    Using PIT Channel 2 is preferred because:
    - It's hardware-driven (no CPU overhead once set up)
    - Produces clean square wave tones
    - Frees the CPU to do other work while sound plays
    
    The speaker is controlled through:
    - Port 0x61: Speaker control register
    - PIT Channel 2 (0x42): Frequency generation
    - PIT Command (0x43): Configure Channel 2 mode
*/

#include "../include/speaker.h"
#include "../../lib/include/io.h"

// Internal state tracking
static uint8_t speaker_initialized = 0;
static uint8_t speaker_playing = 0;
static uint8_t original_speaker_state = 0;

/**
 * Initialize the PC Speaker system
 * Sets up the speaker but leaves it muted initially
 */
void speaker_init(void) {
    if (speaker_initialized) {
        return; // Already initialized
    }
    
    // Read and store the original speaker state
    original_speaker_state = inb(SPEAKER_PORT);
    
    // Forcefully mute the speaker first
    uint8_t current_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current_state & ~(SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    
    // Configure PIT Channel 2 for square wave mode
    // This sets up the timer but doesn't enable the speaker yet
    outb(PIT_COMMAND, PIT_SPEAKER_CMD);
    
    speaker_playing = 0;
    speaker_initialized = 1;
}

/**
 * Calculate the PIT divisor for a given frequency
 * @param frequency Desired frequency in Hz
 * @return PIT divisor value (clamped to valid range)
 */
uint16_t speaker_calculate_divisor(uint32_t frequency) {
    if (frequency < SPEAKER_MIN_FREQ) {
        frequency = SPEAKER_MIN_FREQ;
    } else if (frequency > SPEAKER_MAX_FREQ) {
        frequency = SPEAKER_MAX_FREQ;
    }
    
    uint32_t divisor = SPEAKER_BASE_FREQ / frequency;
    
    // Clamp to 16-bit range
    if (divisor > 0xFFFF) {
        divisor = 0xFFFF;
    } else if (divisor < 1) {
        divisor = 1;
    }
    
    return (uint16_t)divisor;
}

/**
 * Low-level function to set PIT Channel 2 frequency directly
 * @param divisor PIT divisor value (1-65535)
 */
void speaker_set_pit_frequency(uint16_t divisor) {
    // Send the divisor to PIT Channel 2 (low byte first, then high byte)
    outb(PIT_SPEAKER_CHANNEL, divisor & 0xFF);
    outb(PIT_SPEAKER_CHANNEL, (divisor >> 8) & 0xFF);
}

/**
 * Enable the speaker gate (connect PIT Channel 2 to speaker)
 */
void speaker_enable_gate(void) {
    uint8_t current_state = inb(SPEAKER_PORT);
    // Set gate bit (bit 0) to connect PIT Channel 2 to speaker
    // Set data bit (bit 1) to enable speaker output  
    outb(SPEAKER_PORT, current_state | (SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    speaker_playing = 1;
}

/**
 * Disable the speaker gate (disconnect PIT Channel 2 from speaker)
 */
void speaker_disable_gate(void) {
    uint8_t current_state = inb(SPEAKER_PORT);
    // Clear gate bit (bit 0) to disconnect PIT from speaker
    // Keep other bits intact to avoid affecting other hardware
    outb(SPEAKER_PORT, current_state & ~SPEAKER_GATE_BIT);
    speaker_playing = 0;
}

/**
 * Play a tone at the specified frequency
 * @param frequency Frequency in Hz (20-20000 range recommended)
 */
void speaker_play_tone(uint32_t frequency) {
    if (!speaker_initialized) {
        speaker_init();
    }
    
    // Calculate and set the PIT divisor for the desired frequency
    uint16_t divisor = speaker_calculate_divisor(frequency);
    
    // Configure PIT Channel 2 for the new frequency
    outb(PIT_COMMAND, PIT_SPEAKER_CMD);
    speaker_set_pit_frequency(divisor);
    
    // Enable the speaker
    speaker_enable_gate();
}

/**
 * Stop playing sound and mute the speaker
 */
void speaker_stop(void) {
    // First disconnect PIT from speaker
    speaker_disable_gate();
    
    // Then ensure speaker is completely silent by clearing both control bits
    uint8_t current_state = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, current_state & ~(SPEAKER_GATE_BIT | SPEAKER_DATA_BIT));
    
    speaker_playing = 0;
}

/**
 * Check if the speaker is currently playing a tone
 * @return 1 if playing, 0 if muted
 */
uint8_t speaker_is_playing(void) {
    return speaker_playing;
}

/**
 * Simple delay function that doesn't rely on interrupts
 * @param milliseconds Approximate milliseconds to wait
 */
static void speaker_delay(uint32_t milliseconds) {
    // Simple busy wait - approximate 1ms per 1000000 iterations
    // This is very rough and CPU-dependent, but works without interrupts
    volatile uint32_t cycles = milliseconds * 100000;
    for (volatile uint32_t i = 0; i < cycles; i++) {
        // Just burn CPU cycles
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
    speaker_delay(duration_ms);  // Use our own delay instead of pit_wait
    speaker_stop();
}

/**
 * Play a standard system beep (1000Hz for 200ms)
 */
void speaker_system_beep(void) {
    speaker_beep(1000, 200);
}

/**
 * Play an error beep (low frequency, longer duration)
 */
void speaker_error_beep(void) {
    // Play three low beeps for error indication
    speaker_beep(200, 150);
    speaker_delay(50);
    speaker_beep(200, 150);
    speaker_delay(50);
    speaker_beep(200, 300);
}

/**
 * Play a success beep (high frequency, short duration)
 */
void speaker_success_beep(void) {
    // Two quick high beeps for success
    speaker_beep(1500, 100);
    speaker_delay(50);
    speaker_beep(2000, 150);
}

/**
 * Play a musical note by frequency
 * @param note_frequency Use NOTE_* constants or custom frequency
 * @param duration_ms Duration in milliseconds
 */
void speaker_play_note(uint32_t note_frequency, uint32_t duration_ms) {
    speaker_beep(note_frequency, duration_ms);
}

// Convenience functions for common beep patterns

/**
 * Play a startup melody
 */
void speaker_startup_melody(void) {
    speaker_play_note(NOTE_C4, 200);
    speaker_delay(50);
    speaker_play_note(NOTE_E4, 200);
    speaker_delay(50);
    speaker_play_note(NOTE_G4, 200);
    speaker_delay(50);
    speaker_play_note(NOTE_C5, 300);
}
/**
 * Play an attention beep (like a notification)
 */
void speaker_notification_beep(void) {
    speaker_beep(800, 100);
    speaker_delay(100);
    speaker_beep(1200, 150);
}

/**
 * Play a warning beep pattern
 */
void speaker_warning_beep(void) {
    for (int i = 0; i < 5; i++) {
        speaker_beep(400, 80);
        speaker_delay(80);
    }
}

/**
 * Test function to play a scale
 */
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
