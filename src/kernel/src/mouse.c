/*
    MooseOS
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

/*
    ======================== OS THEORY ========================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.
    
    WHAT IS PS/2 MOUSE PROTOCOL?
    PS/2 is an old but still common interface for mice and keyboards.
    
    HOW MICE REPORT MOVEMENT:
    Unlike keyboards that send "key pressed/released", mice continuously report:
    1. RELATIVE MOVEMENT: "I moved 5 pixels left, 3 pixels up"
    2. BUTTON STATES: "Left button is pressed, right button released"
    3. SCROLL WHEEL: "User scrolled up 2 notches"
    
    PS/2 MOUSE DATA PACKET:
    Mice send data in 3-byte packets:
    
    BYTE 1 (Status Byte):
    - Bit 0: Left button pressed (1) or released (0)  
    - Bit 1: Right button pressed (1) or released (0)
    - Bit 2: Middle button pressed (1) or released (0)
    - Bit 4: X movement sign (1 = negative, 0 = positive)
    - Bit 5: Y movement sign (1 = negative, 0 = positive)
    - Other bits: Overflow flags and sync bits
    
    BYTE 2: X movement delta (-256 to +255 pixels)
    BYTE 3: Y movement delta (-256 to +255 pixels)
    
    MOUSE COORDINATE SYSTEM:
    - X increases going RIGHT
    - Y increases going DOWN (opposite of math!)
    - (0,0) is top-left corner of screen
    - Mouse reports RELATIVE movement, OS tracks ABSOLUTE position

    Source: https://wiki.osdev.org/PS/2_Mouse
            https://wiki.osdev.org/Mouse_Input
*/

#include "../include/mouse.h"

// mouse state
static mouse_state_t mouse_state = {0, 0, 0, 0, 0, 320, 240}; // Start at center of 640x480 screen
static unsigned char mouse_cycle = 0;
static signed char mouse_byte[3];

// wait for mouse, yeaah i dont get this either
void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        // wait for output buffer to be full
        while (timeout--) {
            if ((read_port(MOUSE_STATUS) & MOUSE_BBIT) == 1) {
                return;
            }
        }
    } else {
        // wait for input buffer to be empty
        while (timeout--) {
            if ((read_port(MOUSE_STATUS) & MOUSE_ABIT) == 0) {
                return;
            }
        }
    }
}

// write
void mouse_write(unsigned char data) {
    mouse_wait(1);
    write_port(MOUSE_STATUS, MOUSE_WRITE);
    mouse_wait(1);
    write_port(MOUSE_PORT, data);
}

// read
unsigned char mouse_read(void) {
    mouse_wait(0);
    return read_port(MOUSE_PORT);
}

// init
void mouse_init(void) {
    unsigned char status;

    // enable device
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0xA8);

    // enable IRQ
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0x20);
    mouse_wait(0);
    status = read_port(MOUSE_PORT) | 2;
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0x60);
    mouse_wait(1);
    write_port(MOUSE_PORT, status);

    // idk
    mouse_write(0xF6);
    mouse_read(); 

    // enable
    mouse_write(0xF4);
    mouse_read();

    mouse_cycle = 0;
}

// handle interrupts
// Add lock.h for handler_lock
void mouse_handler_main(void) {
    if (handler_lock != 0) return;
    handler_lock = 1;

    unsigned char status = read_port(MOUSE_STATUS);
    
    write_port(0xA0, 0x20); 
    write_port(0x20, 0x20); 
    
    // safety checks
    if (!(status & MOUSE_BBIT)) {
        handler_lock = 0;
        return; // no data
    }

    if (!(status & MOUSE_F_BIT)) {
        read_port(MOUSE_PORT);
        handler_lock = 0;
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
            mouse_state.left_button = mouse_byte[0] & 0x01;
            mouse_state.right_button = (mouse_byte[0] & 0x02) >> 1;
            mouse_state.middle_button = (mouse_byte[0] & 0x04) >> 2;

            // calculate movement
            mouse_state.x_movement = mouse_byte[1];
            mouse_state.y_movement = mouse_byte[2];
            
            if (mouse_byte[0] & 0x10) {
                mouse_state.x_movement |= 0xFFFFFF00;
            }
            if (mouse_byte[0] & 0x20) {
                mouse_state.y_movement |= 0xFFFFFF00;
            }

            #define MAX_MOUSE_SPEED 5
            if (mouse_state.x_movement > MAX_MOUSE_SPEED) mouse_state.x_movement = MAX_MOUSE_SPEED;
            if (mouse_state.x_movement < -MAX_MOUSE_SPEED) mouse_state.x_movement = -MAX_MOUSE_SPEED;
            if (mouse_state.y_movement > MAX_MOUSE_SPEED) mouse_state.y_movement = MAX_MOUSE_SPEED;
            if (mouse_state.y_movement < -MAX_MOUSE_SPEED) mouse_state.y_movement = -MAX_MOUSE_SPEED;

            // update position
            mouse_state.x_position += mouse_state.x_movement;
            mouse_state.y_position -= mouse_state.y_movement;

            // keep mouse within bounds
            if (mouse_state.x_position < 0) mouse_state.x_position = 0;
            if (mouse_state.x_position >= 640) mouse_state.x_position = 639;
            if (mouse_state.y_position < 0) mouse_state.y_position = 0;
            if (mouse_state.y_position >= 480) mouse_state.y_position = 479;
            
            break;
            
        default:
            // invalid, reset
            mouse_cycle = 0;
            break;
    }
    handler_lock = 0;
}

// get mouse state
mouse_state_t* get_mouse_state(void) {
    return &mouse_state;
}
