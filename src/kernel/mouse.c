/*
    MooseOS Mouse Support
    Copyright 2025 Ethan Zhang
*/

// Mouse data packet structure
typedef struct {
    unsigned char left_button;
    unsigned char right_button;
    unsigned char middle_button;
    int x_movement;
    int y_movement;
    int x_position;
    int y_position;
} mouse_state_t;

// Mouse constants
#define MOUSE_PORT   0x60
#define MOUSE_STATUS 0x64
#define MOUSE_ABIT   0x02
#define MOUSE_BBIT   0x01
#define MOUSE_WRITE  0xD4
#define MOUSE_F_BIT  0x20
#define MOUSE_V_BIT  0x08

extern bool dock_is_active(void);
// External functions from boot.asm
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);

// Mouse state
static mouse_state_t mouse_state = {0, 0, 0, 0, 0, 320, 240}; // Start at center of 640x480 screen
static unsigned char mouse_cycle = 0;
static signed char mouse_byte[3];

// Wait for mouse controller
void mouse_wait(unsigned char type) {
    unsigned int timeout = 100000;
    if (type == 0) {
        // Wait for output buffer to be full
        while (timeout--) {
            if ((read_port(MOUSE_STATUS) & MOUSE_BBIT) == 1) {
                return;
            }
        }
    } else {
        // Wait for input buffer to be empty
        while (timeout--) {
            if ((read_port(MOUSE_STATUS) & MOUSE_ABIT) == 0) {
                return;
            }
        }
    }
}

// Write to mouse
void mouse_write(unsigned char data) {
    mouse_wait(1);
    write_port(MOUSE_STATUS, MOUSE_WRITE);
    mouse_wait(1);
    write_port(MOUSE_PORT, data);
}

// Read from mouse
unsigned char mouse_read(void) {
    mouse_wait(0);
    return read_port(MOUSE_PORT);
}

// Initialize mouse
void mouse_init(void) {
    unsigned char status;

    // Enable auxiliary mouse device
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0xA8);

    // Enable mouse IRQ
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0x20);
    mouse_wait(0);
    status = read_port(MOUSE_PORT) | 2;
    mouse_wait(1);
    write_port(MOUSE_STATUS, 0x60);
    mouse_wait(1);
    write_port(MOUSE_PORT, status);

    // Set mouse defaults
    mouse_write(0xF6);
    mouse_read(); // Acknowledge

    // Enable mouse
    mouse_write(0xF4);
    mouse_read(); // Acknowledge

    mouse_cycle = 0;
}

// Handle mouse interrupt
void mouse_handler_main(void) {
    unsigned char status = read_port(MOUSE_STATUS);
    
    // Send EOI to both PICs since mouse is on IRQ12 (slave PIC)
    write_port(0xA0, 0x20); // Send EOI to slave PIC
    write_port(0x20, 0x20); // Send EOI to master PIC
    
    // Quick safety checks to prevent hanging
    if (!(status & MOUSE_BBIT)) {
        return; // No data available
    }

    if (!(status & MOUSE_F_BIT)) {
        // Clear any spurious data to prevent getting stuck
        read_port(MOUSE_PORT);
        return;
    }

    signed char mouse_in = read_port(MOUSE_PORT);

    switch (mouse_cycle) {
        case 0:
            // First byte: button status and signs
            mouse_byte[0] = mouse_in;
            
            // Check if this is a valid first byte
            if (!(mouse_in & MOUSE_V_BIT)) {
                mouse_cycle = 0; // Reset cycle on invalid data
                break;
            }
            
            mouse_cycle++;
            break;
            
        case 1:
            // Second byte: X movement
            mouse_byte[1] = mouse_in;
            mouse_cycle++;
            break;
            
        case 2:
            // Third byte: Y movement
            mouse_byte[2] = mouse_in;
            mouse_cycle = 0; // Always reset cycle after third byte

            // Update mouse state
            mouse_state.left_button = mouse_byte[0] & 0x01;
            mouse_state.right_button = (mouse_byte[0] & 0x02) >> 1;
            mouse_state.middle_button = (mouse_byte[0] & 0x04) >> 2;

            // Calculate movement (handle sign extension)
            mouse_state.x_movement = mouse_byte[1];
            mouse_state.y_movement = mouse_byte[2];
            
            // Apply sign extension if needed
            if (mouse_byte[0] & 0x10) { // X sign bit
                mouse_state.x_movement |= 0xFFFFFF00;
            }
            if (mouse_byte[0] & 0x20) { // Y sign bit
                mouse_state.y_movement |= 0xFFFFFF00;
            }

            // Update position (with bounds checking)
            mouse_state.x_position += mouse_state.x_movement;
            mouse_state.y_position -= mouse_state.y_movement; // Y is inverted

            // Keep mouse within screen bounds (assuming 640x480)
            if (mouse_state.x_position < 0) mouse_state.x_position = 0;
            if (mouse_state.x_position >= 640) mouse_state.x_position = 639;
            if (mouse_state.y_position < 0) mouse_state.y_position = 0;
            if (mouse_state.y_position >= 480) mouse_state.y_position = 479;
            
            // Update cursor only on actual movement to prevent excessive redraws
            static uint32_t last_cursor_update = 0;
            extern volatile uint32_t ticks;
            extern void gui_update_mouse_cursor(void);
            
            if (ticks - last_cursor_update >= 1) { // Limit to every 10ms (more frequent)
                gui_update_mouse_cursor();
                last_cursor_update = ticks;
            }
            
            break;
            
        default:
            // Reset cycle if we get into an invalid state
            mouse_cycle = 0;
            break;
    }
}

// Get current mouse state
mouse_state_t* get_mouse_state(void) {
    return &mouse_state;
}
