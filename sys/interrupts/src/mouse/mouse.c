/*
    MooseOS Mouse interrupt handler code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "mouse/mouse.h"

// mouse state
static mouse_state mouse_states = {0, 0, 0, 0, 0, 320, 240}; // Start at center of 640x480 screen
static unsigned char mouse_cycle = 0;
static signed char mouse_byte[3];

// write
void mouse_write(unsigned char data) {
    write_port(MOUSE_STATUS, MOUSE_WRITE);
    write_port(MOUSE_PORT, data);
}

// read
unsigned char mouse_read(void) {
    return read_port(MOUSE_PORT);
}

// init
void mouse_init(void) {
    unsigned char status;

    // enable device
    write_port(MOUSE_STATUS, 0xA8);

    // enable IRQ
    write_port(MOUSE_STATUS, 0x20);
    status = read_port(MOUSE_PORT) | 2;
    write_port(MOUSE_STATUS, 0x60);
    write_port(MOUSE_PORT, status);

    mouse_write(0xF6);
    mouse_read(); 

    mouse_write(0xF4);
    mouse_read();

    mouse_cycle = 0;
}

// handle interrupts
void mouse_handler_main(void) {

    unsigned char status = read_port(MOUSE_STATUS);
    
    write_port(0xA0, 0x20); 
    write_port(0x20, 0x20); 
    
    // Safety checks
    if (!(status & MOUSE_BBIT)) {
        return; // no data
    }

    if (!(status & MOUSE_F_BIT)) {
        read_port(MOUSE_PORT);
        return;
    }

    signed char mouse_in = read_port(MOUSE_PORT);

    switch (mouse_cycle) {
        case 0:
            // mouse buttons
            mouse_byte[0] = mouse_in;
            
            // check if mouse data is valid
            if (!(mouse_in & MOUSE_V_BIT)) {
                mouse_cycle = 0; // reset cycle if invalid
                break;
            }
            
            mouse_cycle++;
            break;
            
        case 1:
            // x movement
            mouse_byte[1] = mouse_in;
            mouse_cycle++;
            break;
            
        case 2:
            // y movement
            mouse_byte[2] = mouse_in;
            mouse_cycle = 0;

            // update mouse state
            mouse_states.left_button = mouse_byte[0] & 0x01;
            mouse_states.right_button = (mouse_byte[0] & 0x02) >> 1;
            mouse_states.middle_button = (mouse_byte[0] & 0x04) >> 2;

            // calculate movement
            mouse_states.x_movement = mouse_byte[1];
            mouse_states.y_movement = mouse_byte[2];
            
            if (mouse_byte[0] & 0x10) {
                mouse_states.x_movement |= 0xFFFFFF00;
            }
            if (mouse_byte[0] & 0x20) {
                mouse_states.y_movement |= 0xFFFFFF00;
            }

            #define MAX_MOUSE_SPEED 5
            if (mouse_states.x_movement > MAX_MOUSE_SPEED) mouse_states.x_movement = MAX_MOUSE_SPEED;
            if (mouse_states.x_movement < -MAX_MOUSE_SPEED) mouse_states.x_movement = -MAX_MOUSE_SPEED;
            if (mouse_states.y_movement > MAX_MOUSE_SPEED) mouse_states.y_movement = MAX_MOUSE_SPEED;
            if (mouse_states.y_movement < -MAX_MOUSE_SPEED) mouse_states.y_movement = -MAX_MOUSE_SPEED;

            // update position
            mouse_states.x_position += mouse_states.x_movement;
            mouse_states.y_position -= mouse_states.y_movement;

            // keep mouse within bounds
            if (mouse_states.x_position < 0) mouse_states.x_position = 0;
            if (mouse_states.x_position >= 640) mouse_states.x_position = 639;
            if (mouse_states.y_position < 0) mouse_states.y_position = 0;
            if (mouse_states.y_position >= 480) mouse_states.y_position = 479;

            break;
            
        default:
            // invalid; reset
            mouse_cycle = 0;
            break;
    }
}

// get mouse state
mouse_state* get_mouse_state(void) {
    return &mouse_states;
}
