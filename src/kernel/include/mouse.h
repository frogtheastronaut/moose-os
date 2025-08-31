/*
    MooseOS Mouse Support
    Copyright (c) 2025 Ethan Zhang
*/
#ifndef MOUSE_H
#define MOUSE_H

#include "../../lib/lib.h"
typedef struct {
    unsigned char left_button;
    unsigned char right_button;
    unsigned char middle_button;
    int x_movement;
    int y_movement;
    int x_position;
    int y_position;
} mouse_state_t;
mouse_state_t* get_mouse_state(void);

// defs
#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

extern bool dock_is_active(void);
// extern functions from boot.asm
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);

void mouse_init(void);

#endif
