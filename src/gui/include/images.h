#ifndef IMAGES_H
#define IMAGES_H

#include <stdint.h>
#include "../../kernel/include/vga.h"

// Images in ../images.c
extern const uint8_t folder_icon[16][16];
extern const uint8_t file_icon[16][16];
extern const uint8_t terminal_icon[16][16];
extern const uint8_t cursor_icon[8][8];
extern const uint8_t icon_color_map[];

#endif