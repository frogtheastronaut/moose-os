#ifndef FONTDEF_H
#define FONTDEF_H

#include <stdint.h>

// Font definition prototypes.
// The font definitions are in ../fontdef.c
extern const uint8_t system_font[256][8];
extern const uint8_t char_widths[256];

#endif