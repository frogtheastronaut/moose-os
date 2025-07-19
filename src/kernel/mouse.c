/*
    MooseOS Mouse Support
    Copyright 2025 Ethan Zhang
*/


typedef struct {
    unsigned char left_button;
    unsigned char right_button;
    unsigned char middle_button;
    int x_movement;
    int y_movement;
    int x_position;
    int y_position;
} mouse_state_t;

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
void mouse_handler_main(void) {
    unsigned char status = read_port(MOUSE_STATUS);
    
    write_port(0xA0, 0x20); 
    write_port(0x20, 0x20); 
    
    // safety checks
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

            // update position
            mouse_state.x_position += mouse_state.x_movement;
            mouse_state.y_position -= mouse_state.y_movement; // Y is inverted

            // keep mouse within bounds
            if (mouse_state.x_position < 0) mouse_state.x_position = 0;
            if (mouse_state.x_position >= 640) mouse_state.x_position = 639;
            if (mouse_state.y_position < 0) mouse_state.y_position = 0;
            if (mouse_state.y_position >= 480) mouse_state.y_position = 479;
            
            static uint32_t last_cursor_update = 0;
            extern volatile uint32_t ticks;
            extern void gui_updatemouse(void);
            
            if (ticks - last_cursor_update >= 1) {
                gui_updatemouse();
                last_cursor_update = ticks;
            }
            
            break;
            
        default:
            // invalid, reset
            mouse_cycle = 0;
            break;
    }
}

// get mouse state
mouse_state_t* get_mouse_state(void) {
    return &mouse_state;
}
