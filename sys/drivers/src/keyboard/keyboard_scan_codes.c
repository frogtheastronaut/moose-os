/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

#include "../include/keyboard/keyboard_scan_codes.h"

unsigned char keyboard_map_normal[128] = {
    0,      // 0x00: (none)
    27,     // 0x01: ESC
    '1',    // 0x02: 1
    '2',    // 0x03: 2
    '3',    // 0x04: 3
    '4',    // 0x05: 4
    '5',    // 0x06: 5
    '6',    // 0x07: 6
    '7',    // 0x08: 7
    '8',    // 0x09: 8
    '9',    // 0x0A: 9
    '0',    // 0x0B: 0
    '-',    // 0x0C: -
    '=',    // 0x0D: =
    '\b',   // 0x0E: Backspace
    '\t',   // 0x0F: Tab
    'q',    // 0x10: Q
    'w',    // 0x11: W
    'e',    // 0x12: E
    'r',    // 0x13: R
    't',    // 0x14: T
    'y',    // 0x15: Y
    'u',    // 0x16: U
    'i',    // 0x17: I
    'o',    // 0x18: O
    'p',    // 0x19: P
    '[',    // 0x1A: [
    ']',    // 0x1B: ]
    '\n',   // 0x1C: Enter
    0,      // 0x1D: Left Ctrl
    'a',    // 0x1E: A
    's',    // 0x1F: S
    'd',    // 0x20: D
    'f',    // 0x21: F
    'g',    // 0x22: G
    'h',    // 0x23: H
    'j',    // 0x24: J
    'k',    // 0x25: K
    'l',    // 0x26: L
    ';',    // 0x27: ;
    '\'',   // 0x28: '
    '`',    // 0x29: `
    0,      // 0x2A: Left Shift
    '\\',   // 0x2B: Backslash
    'z',    // 0x2C: Z
    'x',    // 0x2D: X
    'c',    // 0x2E: C
    'v',    // 0x2F: V
    'b',    // 0x30: B
    'n',    // 0x31: N
    'm',    // 0x32: M
    ',',    // 0x33: ,
    '.',    // 0x34: .
    '/',    // 0x35: /
    0,      // 0x36: Right Shift
    '*',    // 0x37: Keypad *
    0,      // 0x38: Left Alt
    ' ',    // 0x39: Space
    0,      // 0x3A: Caps Lock
    0,      // 0x3B: F1
    0,      // 0x3C: F2
    0,      // 0x3D: F3
    0,      // 0x3E: F4
    0,      // 0x3F: F5
    0,      // 0x40: F6
    0,      // 0x41: F7
    0,      // 0x42: F8
    0,      // 0x43: F9
    0,      // 0x44: F10
    0,      // 0x45: Num Lock
    0,      // 0x46: Scroll Lock
    0,      // 0x47: Keypad 7 (Home)
    0,      // 0x48: Keypad 8 (Up)
    0,      // 0x49: Keypad 9 (PgUp)
    0,      // 0x4A: Keypad -
    0,      // 0x4B: Keypad 4 (Left)
    0,      // 0x4C: Keypad 5
    0,      // 0x4D: Keypad 6 (Right)
    0,      // 0x4E: Keypad +
    0,      // 0x4F: Keypad 1 (End)
    0,      // 0x50: Keypad 2 (Down)
    0,      // 0x51: Keypad 3 (PgDn)
    0,      // 0x52: Keypad 0 (Ins)
    0,      // 0x53: Keypad . (Del)
    0,      // 0x54: (unused)
    0,      // 0x55: (unused)
    0,      // 0x56: (unused)
    0,      // 0x57: F11
    0,      // 0x58: F12
    0,      // 0x59: (unused)
    0,      // 0x5A: (unused)
    0,      // 0x5B: (unused)
    0,      // 0x5C: (unused)
    0,      // 0x5D: (unused)
    0,      // 0x5E: (unused)
    0,      // 0x5F: (unused)
    0,      // 0x60: (unused)
    0,      // 0x61: (unused)
    0,      // 0x62: (unused)
    0,      // 0x63: (unused)
    0,      // 0x64: (unused)
    0,      // 0x65: (unused)
    0,      // 0x66: (unused)
    0,      // 0x67: (unused)
    0,      // 0x68: (unused)
    0,      // 0x69: (unused)
    0,      // 0x6A: (unused)
    0,      // 0x6B: (unused)
    0,      // 0x6C: (unused)
    0,      // 0x6D: (unused)
    0,      // 0x6E: (unused)
    0,      // 0x6F: (unused)
    0,      // 0x70: (unused)
    0,      // 0x71: (unused)
    0,      // 0x72: (unused)
    0,      // 0x73: (unused)
    0,      // 0x74: (unused)
    0,      // 0x75: (unused)
    0,      // 0x76: (unused)
    0,      // 0x77: (unused)
    0,      // 0x78: (unused)
    0,      // 0x79: (unused)
    0,      // 0x7A: (unused)
    0,      // 0x7B: (unused)
    0,      // 0x7C: (unused)
    0,      // 0x7D: (unused)
    0,      // 0x7E: (unused)
    0       // 0x7F: (unused)
};

// Shifted (shift, no caps)
unsigned char keyboard_map_shift[128] = {
    0,  27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', /* 0x0E: Backspace */
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',   /* 0x1C: Enter */
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0, '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*', 0, ' ', 0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

// Caps Lock (caps, no shift)
unsigned char keyboard_map_caps[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b', /* 0x0E: Backspace */
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',   /* 0x1C: Enter */
    0,  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`', 0, '\\',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/', 0, '*', 0, ' ', 0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};