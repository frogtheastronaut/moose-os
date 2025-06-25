#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "images.h"
#include "fontdef.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h" // Make sure this includes the key code definitions

// Screen buffer in VGA mode 13h (320x200, 256 colors)
static uint8_t* vga_buffer = (uint8_t*)0xA0000;

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

// Add these global variables to track selection state
int current_selection = 0;  // Index of currently selected item
bool explorer_active = false;  // Is the file explorer active?

/**
 * Sets a single pixel at the specified coordinates
 * 
 * @param x X-coordinate (0-319)
 * @param y Y-coordinate (0-199)
 * @param color VGA color value (0-255)
 */
void gui_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a horizontal line from (x1,y) to (x2,y)
 * 
 * @param x1 Starting X-coordinate
 * @param x2 Ending X-coordinate
 * @param y Y-coordinate
 * @param color Line color
 */
void gui_draw_hline(int x1, int x2, int y, uint8_t color) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    
    // Ensure x1 <= x2
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    // Clip to screen boundaries
    if (x1 < 0) x1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    
    // Draw the line
    for (int x = x1; x <= x2; x++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a vertical line from (x,y1) to (x,y2)
 * 
 * @param x X-coordinate
 * @param y1 Starting Y-coordinate
 * @param y2 Ending Y-coordinate
 * @param color Line color
 */
void gui_draw_vline(int x, int y1, int y2, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH) return;
    
    // Ensure y1 <= y2
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    // Clip to screen boundaries
    if (y1 < 0) y1 = 0;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;
    
    // Draw the line
    for (int y = y1; y <= y2; y++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

/**
 * Draws a filled rectangle
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Fill color
 */
void gui_draw_rect(int x, int y, int width, int height, uint8_t color) {
    // Clip rectangle to screen boundaries
    int x_end = x + width;
    int y_end = y + height;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x_end > SCREEN_WIDTH) x_end = SCREEN_WIDTH;
    if (y_end > SCREEN_HEIGHT) y_end = SCREEN_HEIGHT;
    
    // Draw the rectangle
    for (int j = y; j < y_end; j++) {
        for (int i = x; i < x_end; i++) {
            vga_buffer[j * SCREEN_WIDTH + i] = color;
        }
    }
}

/**
 * Draws a rectangle outline
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param color Border color
 */
void gui_draw_rect_outline(int x, int y, int width, int height, uint8_t color) {
    // Draw top and bottom horizontal lines
    gui_draw_hline(x, x + width - 1, y, color);
    gui_draw_hline(x, x + width - 1, y + height - 1, color);
    
    // Draw left and right vertical lines
    gui_draw_vline(x, y, y + height - 1, color);
    gui_draw_vline(x + width - 1, y, y + height - 1, color);
}

/**
 * Draws a 3D-style box with highlights and shadows
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of box
 * @param height Height of box
 * @param face_color Main color of the box
 * @param highlight_color Color for top and left edges (typically lighter)
 * @param shadow_color Color for bottom and right edges (typically darker)
 */
void gui_draw_3d_box(int x, int y, int width, int height, 
                    uint8_t face_color, 
                    uint8_t highlight_color, 
                    uint8_t shadow_color) {
    // Draw the main face
    gui_draw_rect(x + 1, y + 1, width - 2, height - 2, face_color);
    
    // Draw the top and left highlight
    gui_draw_hline(x, x + width - 1, y, highlight_color);
    gui_draw_vline(x, y, y + height - 1, highlight_color);
    
    // Draw the bottom and right shadow
    gui_draw_hline(x + 1, x + width - 1, y + height - 1, shadow_color);
    gui_draw_vline(x + width - 1, y + 1, y + height - 1, shadow_color);
}

/**
 * Draws a double-bordered box (window style)
 * 
 * @param x Left position
 * @param y Top position
 * @param width Width of box
 * @param height Height of box
 * @param outer_color Color for outer border
 * @param inner_color Color for inner border
 * @param face_color Color for inner face
 */
void gui_draw_window_box(int x, int y, int width, int height,
                        uint8_t outer_color,
                        uint8_t inner_color,
                        uint8_t face_color) {
    // Draw outer border
    gui_draw_rect_outline(x, y, width, height, outer_color);
    
    // Draw inner border
    gui_draw_rect_outline(x + 1, y + 1, width - 2, height - 2, inner_color);
    
    // Draw inner face
    gui_draw_rect(x + 2, y + 2, width - 4, height - 4, face_color);
}

/**
 * Clears the entire screen with specified color
 * 
 * @param color Color to fill the screen with
 */
void gui_clear_screen(uint8_t color) {
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        vga_buffer[i] = color;
    }
}

/**
 * Draws a title bar for a window
 * 
 * @param x Left position of window
 * @param y Top position of window
 * @param width Width of window
 * @param title_height Height of title bar
 * @param title_color Background color of title bar
 */
void gui_draw_title_bar(int x, int y, int width, int title_height, uint8_t title_color) {
    // Draw title bar background
    gui_draw_rect(x + 2, y + 2, width - 4, title_height, title_color);
    
    // Draw a line at the bottom of the title bar
    gui_draw_hline(x + 2, x + width - 3, y + title_height + 1, VGA_COLOR_BLACK);
}

// Simple I/O functions if you need to implement them
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * Initialize VGA mode 13h (320x200, 256 colors) using direct VGA register programming
 */
void gui_init() {
    // Reset VGA state
    outb(0x3C2, 0x63);
    
    // Sequence controller registers
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    
    // CRTC controller registers
    // Unlock CRTC registers
    outb(0x3D4, 0x03); outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & ~0x80);
    
    // Program CRTC registers for mode 13h
    static const uint8_t crtc_data[] = {
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF
    };
    
    for (int i = 0; i < 25; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc_data[i]);
    }
    
    // Graphics controller registers for mode 13h
    static const uint8_t gc_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t gc_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};
    
    for (int i = 0; i < 9; i++) {
        outb(0x3CE, gc_reg[i]);
        outb(0x3CF, gc_data[i]);
    }
    
    // Attribute controller registers
    // Reset attribute controller flip-flop to index state
    inb(0x3DA);
    
    static const uint8_t ac_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
    static const uint8_t ac_data[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41, 0x00, 0x0F, 0x00, 0x00
    };
    
    for (int i = 0; i < 21; i++) {
        outb(0x3C0, ac_reg[i]);
        outb(0x3C0, ac_data[i]);
    }
    
    // Enable display
    outb(0x3C0, 0x20);
    
    // Set the buffer pointer
    vga_buffer = (uint8_t*)0xA0000;
}

/**
 * Draws a character at the specified position using proportional spacing
 * 
 * @param x X-coordinate of top-left corner
 * @param y Y-coordinate of top-left corner
 * @param c Character to draw
 * @param color Font color
 */
void gui_draw_char(int x, int y, char c, uint8_t color) {
    // Get the font data
    const uint8_t *glyph = system_font[(unsigned char)c];
    
    // Get the character width
    int char_width = char_widths[(unsigned char)c];
    
    // Calculate any offset for narrow characters (center them in their cell)
    int offset = 0;
    if (char_width < 8) {
        offset = (8 - char_width) / 2;
    }
    
    // Draw each pixel of the character
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = glyph[row];
        
        for (int col = 0; col < char_width; col++) {
            // Check if this pixel should be drawn
            if (row_data & (0x80 >> (col + offset))) {
                gui_set_pixel(x + col, y + row, color);
            }
        }
    }
}

/**
 * Draws text at the specified position with improved spacing
 * 
 * @param x X-coordinate of top-left corner
 * @param y Y-coordinate of top-left corner
 * @param text Text to draw
 * @param color Font color
 */
void gui_draw_text(int x, int y, const char* text, uint8_t color) {
    int current_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        
        // Handle special characters
        if (c == '\n') {
            current_x = x;
            y += 9;  // Slightly more line spacing (was 8)
            continue;
        }
        
        // Draw the character
        gui_draw_char(current_x, y, c, color);
        
        // Move to the next character position using the width table
        current_x += char_widths[c] + 1;  // Add 1 pixel spacing between characters
        
        // Wrap text if it exceeds screen width
        if (current_x >= SCREEN_WIDTH - 8) {
            current_x = x;
            y += 9;  // Slightly more line spacing
        }
    }
}

/**
 * Calculates the pixel width of a text string
 * 
 * @param text The text string
 * @return Width in pixels
 */
int gui_text_width(const char* text) {
    int width = 0;
    
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        width += char_widths[c] + 1;  // Add 1 pixel spacing between characters
    }
    
    // Remove the extra spacing after the last character
    if (width > 0) width--;
    
    return width;
}

/**
 * Draws text centered within a rectangular area with improved spacing
 * 
 * @param x Left position of rectangle
 * @param y Top position of rectangle
 * @param width Width of rectangle
 * @param height Height of rectangle
 * @param text Text to draw
 * @param color Font color
 */
void gui_draw_centered_text(int x, int y, int width, int height, const char* text, uint8_t color) {
    // Calculate text width using our new function
    int text_width = gui_text_width(text);
    
    // Calculate centered position
    int text_x = x + (width - text_width) / 2;
    int text_y = y + (height - 8) / 2;
    
    // Draw the text
    gui_draw_text(text_x, text_y, text, color);
}

/**
 * Draw a bitmap icon with background color
 * 
 * @param x X-coordinate for top-left corner
 * @param y Y-coordinate for top-left corner
 * @param icon The bitmap icon data
 * @param width The icon width
 * @param height The icon height
 * @param bg_color Background color to use instead of transparency
 */
void gui_draw_icon(int x, int y, const uint8_t icon[][16], int width, int height, uint8_t bg_color) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t pixel = icon[row][col];
            uint8_t color;
            
            if (pixel == 0) {
                // Use background color instead of transparency
                color = bg_color;
            } else {
                color = icon_color_map[pixel];
            }
            
            gui_set_pixel(x + col, y + row, color);
        }
    }
}

/**
 * Draw a file or folder icon with highlight support - FIXED VERSION
 */
void gui_draw_file_item(int x, int y, const char* name, int is_dir, int is_selected) {
    // Calculate text dimensions FIRST
    int name_width = gui_text_width(name);
    // Center the text under the icon, but with a max width of 64px
    int text_x = x + (16 - name_width) / 2;
    if (text_x < x - 24) text_x = x - 24; // Don't let text go too far left
    
    // If selected, draw a highlight rectangle behind the text ONLY (not the icon)
    if (is_selected) {
        // Draw blue highlight box behind the name
        int text_box_width = name_width + 4; // Add padding
        int text_box_height = 10; // Text height + padding
        gui_draw_rect(text_x - 2, y + 17, text_box_width, text_box_height, VGA_COLOR_BLUE);
        
        // Draw a border around the text box for better visibility
        gui_draw_rect_outline(text_x - 2, y + 17, text_box_width, text_box_height, VGA_COLOR_WHITE);
    }
    
    // Draw the appropriate icon (ALWAYS unselected)
    if (is_dir) {
        gui_draw_icon(x, y, folder_icon, 16, 16, VGA_COLOR_LIGHT_GREY);
    } else {
        gui_draw_icon(x, y, file_icon, 16, 16, VGA_COLOR_LIGHT_GREY);
    }
    
    // Use white text if selected, black otherwise
    uint8_t text_color = is_selected ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
    
    // Draw the name
    gui_draw_text(text_x, y + 18, name, text_color);
}

/**
 * Updated file explorer function that shows current selection
 */
void gui_draw_filesplorer() {
    gui_init();
    // Clear screen with a nice background color
    gui_clear_screen(VGA_COLOR_LIGHT_GREY);
    
    // Draw a file explorer window
    gui_draw_window_box(10, 10, 300, 180, 
                       VGA_COLOR_BLACK,
                       VGA_COLOR_WHITE,
                       VGA_COLOR_LIGHT_GREY);
                       
    // Add a title bar to the window
    gui_draw_title_bar(10, 10, 300, 15, VGA_COLOR_BLUE);
    char path_text[64] = "File Explorer - ";
    gui_draw_text(15, 13, path_text, VGA_COLOR_WHITE);
    
    // Display current directory path
    if (cwd == root) {
        // We're at root directory
        gui_draw_text(15 + gui_text_width(path_text), 13, "/", VGA_COLOR_WHITE);
    } else {
        // We're in a subdirectory - build path from bottom up
        char full_path[128] = "";
        char temp[128] = "";
        FileSystemNode* node = cwd;
        
        // Traverse up the directory tree
        while (node != NULL && node != root) {
            // Prepend "/name" to the path
            if (full_path[0] == '\0') {
                // First iteration, just use the name
                copyStr(full_path, node->name);
            } else {
                // Subsequent iterations, prepend with slash
                copyStr(temp, "/");
                strcat(temp, node->name);
                strcat(temp, "/");
                strcat(temp, full_path);
                copyStr(full_path, temp);
            }
            node = node->parent;
        }
        
        // Add leading slash for absolute path
        copyStr(temp, "/");
        strcat(temp, full_path);
        copyStr(full_path, temp);
        
        // Draw the path
        gui_draw_text(15 + gui_text_width(path_text), 13, full_path, VGA_COLOR_WHITE);
    }
    
    // Draw files and folders from the actual filesystem
    int x_pos = 30;
    int y_pos = 40;
    int count = 0;
    
    // Draw parent directory if not at root
    if (cwd != root) {
        // Use the is_selected parameter
        gui_draw_file_item(x_pos, y_pos, "..", 1, current_selection == 0);
        x_pos += 70;
        count++;
        
        // Start a new row if needed
        if (count % 4 == 0) {
            x_pos = 30;
            y_pos += 40;
        }
    }
    
    // Draw actual files and folders
    for (int i = 0; i < cwd->folder.childCount && count < 12; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        
        // Determine if this item is selected
        int selection_index = count;
        if (cwd != root) selection_index += 1;  // Account for ".." entry
        
        // Use the is_selected parameter
        gui_draw_file_item(x_pos, y_pos, child->name, 
                        (child->type == FOLDER_NODE), 
                        current_selection == selection_index);
        
        x_pos += 70; // Move to the next position
        count++;
        
        // Start a new row if needed
        if (count % 4 == 0) {
            x_pos = 30;
            y_pos += 40;
        }
    }
    
    // Add a status bar at the bottom
    gui_draw_rect(10, 175, 300, 15, VGA_COLOR_DARK_GREY);
    
    // Display number of items
    char count_str[16]; // Buffer for the count
    int_to_str(cwd->folder.childCount, count_str, sizeof(count_str));
    
    char status_text[32] = "";
    copyStr(status_text, count_str);
    
    // Append " items" to the count
    if (cwd->folder.childCount == 1) {
        strcat(status_text, " item");
    } else {
        strcat(status_text, " items");
    }
    
    gui_draw_text(15, 178, status_text, VGA_COLOR_WHITE);
    
    // Display keyboard controls in status bar
    //gui_draw_text(210, 178, "ARROWS:Move ENTER:Open", VGA_COLOR_WHITE);
    
    // Set explorer as active
    explorer_active = true;
}

/**
 * Process keyboard input for file explorer with improved selection handling
 */
void gui_handle_explorer_key(unsigned char key, char scancode) {
    if (!explorer_active) return;
    
    int items_per_row = 4;
    int total_items = cwd->folder.childCount;
    if (cwd != root) total_items++; // Account for ".." folder
    
    // Track previous selection to know what to redraw
    int previous_selection = current_selection;
    
    // Process key input (now using constants from keyboard.h)
    switch (scancode) {
        case ARROW_UP_KEY:
            if (current_selection >= items_per_row) {
                current_selection -= items_per_row;
            }
            break;
            
        case ARROW_DOWN_KEY:
            if (current_selection + items_per_row < total_items) {
                current_selection += items_per_row;
            }
            break;
            
        case ARROW_LEFT_KEY:
            if (current_selection % items_per_row > 0) {
                current_selection--;
            }
            break;
            
        case ARROW_RIGHT_KEY:
            if (current_selection % items_per_row < items_per_row - 1 && 
                current_selection + 1 < total_items) {
                current_selection++;
            }
            break;
        case ENTER_KEY_CODE:
            // Navigate to folder or open file
            if (cwd != root && current_selection == 0) {
                // Go to parent directory
                filesys_cd("..");
                current_selection = 0;
                gui_draw_filesplorer(); // Redraw the whole UI
                return;
            } else {
                // Determine actual index in children array
                int actual_index = current_selection;
                if (cwd != root) actual_index -= 1; // Account for ".." entry
                
                if (actual_index >= 0 && actual_index < cwd->folder.childCount) {
                    FileSystemNode* child = cwd->folder.children[actual_index];
                    if (child->type == FOLDER_NODE) {
                        // Navigate to folder
                        filesys_cd(child->name);
                        current_selection = 0;
                        gui_draw_filesplorer(); // Redraw the whole UI
                        return;
                    } else {
                        // For files, you could show a file preview or properties
                        // For now, we'll just highlight it
                    }
                }
            }
            break;
    }
    
    // Only update if selection changed
    if (previous_selection != current_selection) {
        // Calculate grid positions
        int prev_row = previous_selection / items_per_row;
        int prev_col = previous_selection % items_per_row;
        int new_row = current_selection / items_per_row;
        int new_col = current_selection % items_per_row;
        
        // Calculate pixel positions
        int prev_x = 30 + prev_col * 70;
        int prev_y = 40 + prev_row * 40;
        int new_x = 30 + new_col * 70;
        int new_y = 40 + new_row * 40;
        
        // IMPROVED APPROACH: Clear and redraw entire grid cells for both old and new selections
        
        // Clear entire grid cells - use WIDER rectangles to ensure all text highlights are removed
        // This makes sure even long filenames have their highlights completely cleared
        gui_draw_rect(prev_x - 30, prev_y + 17, 70, 10, VGA_COLOR_LIGHT_GREY);
        gui_draw_rect(new_x - 30, new_y + 17, 70, 10, VGA_COLOR_LIGHT_GREY);
        
        // Redraw previous item with no selection
        if (previous_selection == 0 && cwd != root) {
            gui_draw_file_item(prev_x, prev_y, "..", 1, 0);
        } else {
            int idx = previous_selection;
            if (cwd != root) idx--;
            
            if (idx >= 0 && idx < cwd->folder.childCount) {
                FileSystemNode* child = cwd->folder.children[idx];
                gui_draw_file_item(prev_x, prev_y, child->name, (child->type == FOLDER_NODE), 0);
            }
        }
        
        // Draw new item with selection
        if (current_selection == 0 && cwd != root) {
            gui_draw_file_item(new_x, new_y, "..", 1, 1);
        } else {
            int idx = current_selection;
            if (cwd != root) idx--;
            
            if (idx >= 0 && idx < cwd->folder.childCount) {
                FileSystemNode* child = cwd->folder.children[idx];
                gui_draw_file_item(new_x, new_y, child->name, (child->type == FOLDER_NODE), 1);
            }
        }
    }
}

