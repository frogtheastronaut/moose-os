#include <stdint.h>
#include <stddef.h>
#include "../kernel/include/vga.h"
#include "images.h"
#include "fontdef.h"
#include "../kernel/include/keydef.h"
#include "../filesys/file.h"
#include "../kernel/include/keyboard.h" // Make sure this includes the key code definitions
#include "../lib/lib.h" // For string utilities
// Screen buffer in VGA mode 13h (320x200, 256 colors)
static uint8_t* vga_buffer = (uint8_t*)0xA0000;
// Removed horizontal scrolling variables since they are no longer needed
#define EDITOR_MAX_CHARS_PER_LINE 35 // Maximum characters visible per line
// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

uint32_t ticks = 0;

void timer_handler() {
    ticks++;
}

uint32_t get_ticks(void) {
    return ticks;
}
// Add these global variables to track selection state
int current_selection = 0;  // Index of currently selected item

// Maximum length for input string in dialog
#define MAX_DIALOG_INPUT_LEN 128
extern bool dialog_active;
extern bool explorer_active;
// Dialog variables
char dialog_input[MAX_DIALOG_INPUT_LEN + 1] = "";
int dialog_input_pos = 0;
int dialog_type = 0; // 0 = directory creation, 1 = file creation

// Add these variables to the top of gui.c with your other globals
int path_scroll_offset = 0;
uint32_t last_scroll_time = 0;
#define SCROLL_DELAY 15  // Ticks between scroll updates
#define PATH_MAX_WIDTH 160 // Maximum width for path display in pixels

// Text editor state variables
bool editor_active = false;
char editor_content[MAX_CONTENT] = "";
char editor_filename[MAX_NAME_LEN] = "";
int editor_cursor_pos = 0;
int editor_scroll_line = 0; // First visible line
int editor_cursor_line = 0; // Current cursor line
int editor_cursor_col = 0;  // Current cursor column
bool editor_modified = false; // Track if content has been modified

// Text editor constants
#define EDITOR_LINES_VISIBLE 15  // Number of lines visible in editor
#define EDITOR_LINE_HEIGHT 12    // Height of each line in pixels
#define EDITOR_CHAR_WIDTH 8      // Width of each character
#define EDITOR_START_X 15        // Left margin
#define EDITOR_START_Y 40        // Top margin
#define EDITOR_WIDTH 290         // Editor content width

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
 * Draw a file or folder icon with better vertical alignment
 */
void gui_draw_file_item(int x, int y, const char* name, int is_dir, int is_selected) {
    // Define the item dimensions and spacing
    int item_width = 60;
    int item_height = 40;
    int icon_width = 16;
    int icon_height = 16;
    
    // Calculate center position
    int center_x = x + item_width/2;
    
    // If selected, draw a highlight rectangle around the entire item
    if (is_selected) {
        // Draw blue highlight box behind everything
        gui_draw_rect(x, y, item_width, item_height, VGA_COLOR_BLUE);
    }
    
    // Calculate icon position (centered horizontally)
    int icon_x = center_x - (icon_width/2);
    int icon_y = y + 4;  // Fixed distance from the top
    
    // Draw the appropriate icon
    if (is_dir) {
        gui_draw_icon(icon_x, icon_y, folder_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY);
    } else {
        gui_draw_icon(icon_x, icon_y, file_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY);
    }
    
    // Use white text if selected, black otherwise
    uint8_t text_color = is_selected ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
    uint8_t bg_color = is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY;
    
    // Calculate text position (2 pixels below the icon)
    int text_y = icon_y + icon_height + 2;
    
    // Draw the name with truncation if needed
    int name_width = gui_text_width(name);
    int max_name_width = item_width - 4; // Max width, leaving 2px padding on each side
    
    if (name_width <= max_name_width) {
        // Name fits, center it horizontally
        int text_x = center_x - (name_width / 2);
        gui_draw_text(text_x, text_y, name, text_color);
    } else {
        // Name is too long, truncate with ellipsis
        char truncated[64];
        int i = 0;
        int current_width = 0;
        
        // Copy characters until we're about to exceed max width
        while (name[i] != '\0' && 
               current_width + char_widths[(unsigned char)name[i]] < max_name_width - gui_text_width("..")) {
            truncated[i] = name[i];
            current_width += char_widths[(unsigned char)name[i]] + 1;
            i++;
        }
        
        // Add ellipsis and terminate string
        truncated[i] = '.';
        truncated[i+1] = '.';
        truncated[i+2] = '\0';
        
        // Center the truncated text
        int text_x = center_x - (current_width + gui_text_width("..")) / 2;
        
        // Draw background rect to cover any previous text
        gui_draw_rect(text_x - 1, text_y, max_name_width + 2, 10, bg_color);
        
        // Draw the truncated text
        gui_draw_text(text_x, text_y, truncated, text_color);
    }
}
/**
 * Draw scrolling text that animates if too long for the available space
 */
void gui_draw_scrolling_text(int x, int y, const char* text, int max_width, uint8_t color, uint8_t bg_color) {
    // Get the text width
    int text_width = gui_text_width(text);
    
    // If text fits, just draw it normally
    if (text_width <= max_width) {
        gui_draw_text(x, y, text, color);
        return;
    }
    
    // Text is too long - need to truncate the left side
    // Create a clipping box for the text
    gui_draw_rect(x, y, max_width, 10, bg_color);
    
    // Find the starting point in the original string
    int text_len = strlen(text);
    int i = 0;
    int current_width = text_width;
    
    // Skip characters from the start until we're within the max width
    // We leave space for "..." at the beginning
    int ellipsis_width = gui_text_width("...");
    while (i < text_len && current_width > max_width - ellipsis_width) {
        current_width -= (char_widths[(unsigned char)text[i]] + 1);
        i++;
    }
    
    // Draw the ellipsis to indicate truncation
    gui_draw_text(x, y, "...", color);
    
    // Draw the truncated text starting from position i
    gui_draw_text(x + ellipsis_width, y, &text[i], color);
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
    char full_path[128] = "";
    if (cwd == root) {
        // We're at root directory
        copyStr(full_path, "/");
    } else {
        // We're in a subdirectory - build path from bottom up
        FileSystemNode* node = cwd;
        
        // Start with current directory
        copyStr(full_path, node->name);
        node = node->parent;
        
        // Traverse up the directory tree
        while (node != NULL && node != root) {
            // Prepend parent directory with slash
            char temp[128] = "";
            copyStr(temp, node->name);
            strcat(temp, "/");
            strcat(temp, full_path);
            copyStr(full_path, temp);
            
            node = node->parent;
        }
        
        // Add leading slash for absolute path
        char temp[128] = "/";
        strcat(temp, full_path);
        copyStr(full_path, temp);
    }
    
    // Draw the path with scrolling if needed
    int path_x = 15 + gui_text_width(path_text);
    int available_width = 300 - path_x - 10; // Width available for path display
    gui_draw_scrolling_text(path_x, 13, full_path, available_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    
    // Draw files and folders from the actual filesystem
    int x_pos = 30;
    int y_pos = 40;
    int displayed_count = 0;  // This tracks the display position
    
    // Draw parent directory if not at root
    if (cwd != root) {
        // Use the is_selected parameter - this item is at visual position 0
        gui_draw_file_item(x_pos, y_pos, "..", 1, current_selection == 0);
        x_pos += 70;
        displayed_count++;
        
        // Start a new row if needed
        if (displayed_count % 4 == 0) {
            x_pos = 30;
            y_pos += 40;
        }
    }
    
    // Draw actual files and folders
    for (int i = 0; i < cwd->folder.childCount && displayed_count < 12; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        
        // Calculate selection index properly:
        // If in root: selection index = the file index
        // If in subfolder: selection index = file index + 1 (for ".." entry)
        int selection_index = i;
        if (cwd != root) {
            selection_index = i + 1;  // Account for ".." entry
        }
        
        // Use the is_selected parameter
        gui_draw_file_item(x_pos, y_pos, child->name, 
                        (child->type == FOLDER_NODE), 
                        current_selection == selection_index);
        
        x_pos += 70; // Move to the next position
        displayed_count++;
        
        // Start a new row if needed
        if (displayed_count % 4 == 0) {
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
 * Create new directory or file based on dialog input
 */
void dialog_create_item() {
    if (dialog_input[0] != '\0') {
        // Store the current number of items before we create the new one
        int previous_item_count = cwd->folder.childCount;
        
        if (dialog_type == 0) {
            // Create directory
            filesys_mkdir(dialog_input);
        } else {
            // Create file
            filesys_mkfile(dialog_input, "");
        }
        
        // If a new item was actually created, the count will have increased
        if (cwd->folder.childCount > previous_item_count) {
            // Calculate the selection index for the newly created item
            // The newly created item is always the last item in the children array
            if (cwd != root) {
                // In a subdirectory: The new item's index is childCount - 1 (last item)
                // But we need to add 1 for the ".." entry at the beginning
                current_selection = (cwd->folder.childCount - 1) + 1;
            } else {
                // In root, no ".." entry, so last item index is just childCount - 1
                current_selection = cwd->folder.childCount - 1;
            }
            
            // Make sure selection doesn't exceed the visible items limit (12)
            int max_visible = 12;
            if (cwd != root) {
                // If we're in a subdirectory, account for ".." taking one slot
                max_visible = 11;
            }
            
            // If we exceed the visible count, just select the first item
            if (current_selection >= max_visible) {
                current_selection = 0;
            }
        } else {
            // No new item was created (maybe due to duplicate name)
            // Just reset selection to the first item
            current_selection = 0;
        }
        
        // Redraw the file explorer to show the new item
        gui_draw_filesplorer();
    }
}
/**
 * Draws a simple dialog box 
 */
void gui_draw_dialog(const char* title, const char* prompt) {
    // Dialog dimensions
    int width = 200;
    int height = 80;
    int x = (SCREEN_WIDTH - width) / 2;
    int y = (SCREEN_HEIGHT - height) / 2;
    
    // Draw dialog box with shadow
    gui_draw_rect(x + 4, y + 4, width, height, VGA_COLOR_DARK_GREY); // Shadow
    gui_draw_window_box(x, y, width, height,
                      VGA_COLOR_BLACK,
                      VGA_COLOR_WHITE,
                      VGA_COLOR_LIGHT_GREY);
    
    // Draw title bar
    gui_draw_title_bar(x, y, width, 15, VGA_COLOR_BLUE);
    gui_draw_text(x + 10, y + 4, title, VGA_COLOR_WHITE);
    
    // Draw prompt
    gui_draw_text(x + 10, y + 25, prompt, VGA_COLOR_BLACK);

    // Draw input box
    int input_box_width = width - 20;
    gui_draw_rect(x + 10, y + 40, input_box_width, 14, VGA_COLOR_WHITE);
    gui_draw_rect_outline(x + 10, y + 40, input_box_width, 14, VGA_COLOR_BLACK);
    
    // Calculate if text needs scrolling
    int text_width = gui_text_width(dialog_input);
    int max_visible_width = input_box_width - 4;  // Leave room for cursor
    
    if (text_width <= max_visible_width) {
        // Text fits, draw it normally
        gui_draw_text(x + 12, y + 42, dialog_input, VGA_COLOR_BLACK);
        
        // Draw cursor
        if ((get_ticks() / 10) % 2 == 0) { 
            int cursor_x = x + 12 + text_width;
            gui_draw_vline(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
        }
    } else {
        // Text doesn't fit, scroll it so the cursor is visible
        int offset = text_width - max_visible_width;
        
        // Create a substring starting from the offset
        char visible_text[MAX_DIALOG_INPUT_LEN + 1];
        int visible_start = 0;
        
        // Find the starting character that will make the cursor visible
        int current_width = 0;
        for (int i = 0; dialog_input[i] != '\0'; i++) {
            current_width += char_widths[(unsigned char)dialog_input[i]] + 1;
            if (current_width > offset) {
                visible_start = i;
                break;
            }
        }
        
        // Draw the text starting from visible_start
        gui_draw_text(x + 12, y + 42, &dialog_input[visible_start], VGA_COLOR_BLACK);
        
        // Draw cursor at the end
        if ((get_ticks() / 10) % 2 == 0) {
            int cursor_x = x + 12 + (text_width - offset);
            gui_draw_vline(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
        }
    }
    
    // Draw keyboard shortcut text at the bottom
    gui_draw_text(x + 10, y + 62, "ENTER: OK", VGA_COLOR_DARK_GREY);
    gui_draw_text(x + width - 70, y + 62, "ESC: Cancel", VGA_COLOR_DARK_GREY);
}
/**
 * Handle keyboard input for the dialog box
 */
bool gui_handle_dialog_key(unsigned char key, char scancode) {
    if (!dialog_active) return false;
    
    // Handle special keys
    if (scancode == ENTER_KEY_CODE) {
        // User pressed Enter - create the item
        dialog_active = false;
        dialog_create_item();
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        gui_draw_filesplorer();  // Redraw explorer after item creation
        return true;
    } 
    else if (scancode == ESC_KEY_CODE) {
        // User pressed Escape - cancel
        dialog_active = false;
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        gui_draw_filesplorer();
        return true;
    }
    else if (scancode == BS_KEY_CODE) {
        // Backspace - delete last character
        if (dialog_input_pos > 0) {
            dialog_input_pos--;
            dialog_input[dialog_input_pos] = '\0';
            
            // Redraw the dialog
            if (dialog_type == 0) {
                gui_draw_dialog("Create Directory", "Enter directory name:");
            } else {
                gui_draw_dialog("Create File", "Enter file name:");
            }
        }
        return true;
    }
    
    // Use the character key that was already converted in keyhandler.c
    if (key >= 32 && dialog_input_pos < MAX_DIALOG_INPUT_LEN) {
        dialog_input[dialog_input_pos] = key;
        dialog_input_pos++;
        dialog_input[dialog_input_pos] = '\0';
        
        // Redraw the dialog
        if (dialog_type == 0) {
            gui_draw_dialog("Create Directory", "Enter directory name:");
        } else {
            gui_draw_dialog("Create File", "Enter file name:");
        }
        
        return true;
    }
    
    return true; // Consume all keys when dialog is active
}

/**
 * Helper function to get file content by name
 */
char* get_file_content(const char* filename) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, filename)) {
            return child->file.content;
        }
    }
    return NULL;
}

/**
 * Helper function to count lines in text
 */
int count_lines(const char* text) {
    int lines = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }
    return lines;
}

/**
 * Get start of specific line in text
 */
const char* get_line_start(const char* text, int line_num) {
    if (line_num == 0) return text;
    
    int current_line = 0;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            current_line++;
            if (current_line == line_num) {
                return &text[i + 1];
            }
        }
    }
    return text; // Return start if line not found
}

/**
 * Get length of specific line (excluding newline)
 */
int get_line_length(const char* line_start) {
    int length = 0;
    while (line_start[length] && line_start[length] != '\n') {
        length++;
    }
    return length;
}

/**
 * Convert cursor position to line and column
 */
void cursor_pos_to_line_col(int pos, int* line, int* col) {
    *line = 0;
    *col = 0;
    
    for (int i = 0; i < pos && editor_content[i]; i++) {
        if (editor_content[i] == '\n') {
            (*line)++;
            *col = 0;
        } else {
            (*col)++;
        }
    }
}

/**
 * Convert line and column to cursor position
 */
int line_col_to_cursor_pos(int line, int col) {
    int pos = 0;
    int current_line = 0;
    
    // Move to the start of the target line
    while (current_line < line && editor_content[pos]) {
        if (editor_content[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    // Move to the target column
    int current_col = 0;
    while (current_col < col && editor_content[pos] && editor_content[pos] != '\n') {
        pos++;
        current_col++;
    }
    
    return pos;
}


/**
 * Draws the text editor interface
 */
void gui_draw_text_editor() {
    // Clear screen
    gui_draw_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, VGA_COLOR_LIGHT_GREY);
    
    // Draw title bar
    gui_draw_title_bar(0, 0, SCREEN_WIDTH, 15, VGA_COLOR_BLUE);
    
    // Draw "Text Editor - " label
    char label[] = "Text Editor - ";
    gui_draw_text(10, 6, label, VGA_COLOR_WHITE);
    
    // Draw filename with scrolling if it's too long
    int label_width = gui_text_width(label);
    int filename_x = 10 + label_width;
    int max_filename_width = SCREEN_WIDTH - filename_x - 10; // Leave 10px margin on right
    gui_draw_scrolling_text(filename_x, 6, editor_filename, max_filename_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);

    
    // Draw line numbers background
    gui_draw_rect(10, 40, 25, 150, VGA_COLOR_LIGHT_GREY);
    gui_draw_vline(35, 40, 190, VGA_COLOR_DARK_GREY);
    
    // Draw text content
    int y_pos = EDITOR_START_Y + 5;
    int max_lines = EDITOR_LINES_VISIBLE;
    int total_lines = count_lines(editor_content);
    
    // Adjust scroll if cursor is out of view
    if (editor_cursor_line < editor_scroll_line) {
        editor_scroll_line = editor_cursor_line;
    } else if (editor_cursor_line >= editor_scroll_line + max_lines) {
        editor_scroll_line = editor_cursor_line - max_lines + 1;
    }
    
    // Draw visible lines
    for (int line = 0; line < max_lines && (editor_scroll_line + line) < total_lines; line++) {
        int actual_line = editor_scroll_line + line;
        
        // Draw line number
        char line_num[8];
        int_to_str(actual_line + 1, line_num, sizeof(line_num));
        gui_draw_text(12, y_pos, line_num, VGA_COLOR_DARK_GREY);
        
        // Get line content
        const char* line_start = get_line_start(editor_content, actual_line);
        int line_length = get_line_length(line_start);
        
        // Create line buffer with wrapping
        char line_buffer[EDITOR_MAX_CHARS_PER_LINE + 1];
        int chars_to_draw = 0;
        
        // Copy characters up to the maximum line width
        for (int i = 0; i < EDITOR_MAX_CHARS_PER_LINE && i < line_length; i++) {
            line_buffer[chars_to_draw] = line_start[i];
            chars_to_draw++;
        }
        line_buffer[chars_to_draw] = '\0';
        
        // Draw line content
        gui_draw_text(EDITOR_START_X + 25, y_pos, line_buffer, VGA_COLOR_BLACK);
        
        // Draw cursor if on this line
        if (actual_line == editor_cursor_line) {
            // Calculate cursor x position based on actual character widths
            int cursor_x = EDITOR_START_X + 25;
            int cursor_col_to_show = editor_cursor_col;
            
            // If cursor is beyond visible area, don't show it (it's wrapped to next line)
            if (cursor_col_to_show <= EDITOR_MAX_CHARS_PER_LINE) {
                // Add up the widths of characters before the cursor position
                for (int i = 0; i < cursor_col_to_show && i < chars_to_draw; i++) {
                    cursor_x += char_widths[(unsigned char)line_buffer[i]] + 1; // +1 for character spacing
                }
                
                if ((get_ticks() / 15) % 2 == 0) { // Blinking cursor
                    gui_draw_vline(cursor_x, y_pos, y_pos + 10, VGA_COLOR_BLACK);
                }
            }
        }
        
        y_pos += EDITOR_LINE_HEIGHT;
    }
    
    // Draw status bar
    gui_draw_rect(0, 190, SCREEN_WIDTH, 10, VGA_COLOR_DARK_GREY);
    
    // Show cursor position and scroll info
    char status[64] = "Line: ";
    char line_str[8], col_str[8];
    int_to_str(editor_cursor_line + 1, line_str, sizeof(line_str));
    int_to_str(editor_cursor_col + 1, col_str, sizeof(col_str));
    strcat(status, line_str);
    strcat(status, ", Col: ");
    strcat(status, col_str);
    
    gui_draw_text(5, 192, status, VGA_COLOR_WHITE);
    
    // Show file size
    char file_size_str[32] = "File size: ";
    char size_str[16];
    int_to_str(strlen(editor_content), size_str, sizeof(size_str));
    strcat(file_size_str, size_str);
    strcat(file_size_str, " bytes");
    gui_draw_text(150, 192, file_size_str, VGA_COLOR_WHITE);
}

/**
 * Opens a file in the text editor
 */
void gui_open_text_editor(const char* filename) {
    // Copy filename and load content
    copyStr(editor_filename, filename);
    
    // Get file content
    char* content = get_file_content(filename);
    if (content) {
        // Copy content to editor buffer
        int i = 0;
        while (content[i] && i < MAX_CONTENT - 1) {
            editor_content[i] = content[i];
            i++;
        }
        editor_content[i] = '\0';
    } else {
        // File not found or empty, start with empty content
        editor_content[0] = '\0';
    }
    
    // Reset editor state
    editor_cursor_pos = 0;
    editor_scroll_line = 0;
    cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
    editor_modified = false;
    
    // Activate editor and deactivate explorer
    editor_active = true;
    explorer_active = false;
    
    // Draw the editor
    gui_draw_text_editor();
}


/**
 * Process keyboard input for file explorer
 * 
 * @return true if the key was handled, false otherwise
 */
bool gui_handle_explorer_key(unsigned char key, char scancode) {
    // If dialog is active, let the dialog handle the key
    if (dialog_active) {
        return gui_handle_dialog_key(key, scancode);
    }
    
    if (!explorer_active) return false;
    
    int items_per_row = 4;
    int total_items = cwd->folder.childCount;
    if (cwd != root) total_items++; // Account for ".." folder
    
    // Track previous selection to know what to redraw
    int previous_selection = current_selection;
    
    // Process key input
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
                return true;
            } else {
                // Determine actual index in children array
                int actual_index = current_selection;
                if (cwd != root) actual_index -= 1;  // Account for ".." entry
                
                if (actual_index >= 0 && actual_index < cwd->folder.childCount) {
                    FileSystemNode* child = cwd->folder.children[actual_index];
                    if (child->type == FOLDER_NODE) {
                        // Navigate to folder
                        filesys_cd(child->name);
                        current_selection = 0;
                        gui_draw_filesplorer(); // Redraw the whole UI
                        return true;
                    } else {
                        // Open file in text editor
                        gui_open_text_editor(child->name);
                        return true;
                    }
                }
            }
            break;
            
        case 0x20: // 'D' key scancode should be 0x20 in your keyboard map
            // Show directory creation dialog
            dialog_active = true;
            dialog_type = 0; // Directory
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            gui_draw_dialog("Create Directory", "Enter directory name:");
            return true;

        case 0x21: // 'F' key scancode should be 0x21 in your keyboard map
            // Show file creation dialog
            dialog_active = true;
            dialog_type = 1; // File
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            gui_draw_dialog("Create File", "Enter file name:");
            return true;

        case BS_KEY_CODE:
            // Delete the currently selected item (except ".." which can't be deleted)
            if (cwd != root && current_selection == 0) {
                // Can't delete the ".." entry
                return true;
            } else {
                // Determine actual index in children array
                int actual_index = current_selection;
                if (cwd != root) actual_index -= 1;  // Account for ".." entry
                
                if (actual_index >= 0 && actual_index < cwd->folder.childCount) {
                    FileSystemNode* child = cwd->folder.children[actual_index];
                    
                    if (child->type == FOLDER_NODE) {
                        // Delete folder
                        filesys_rmdir(child->name);
                    } else {
                        // Delete file
                        filesys_rm(child->name);
                    }
                    
                    // If there are still items left after deletion, ensure selection is within bounds
                    if (cwd->folder.childCount > 0) {
                        // If we deleted the last item, move selection to the previous item
                        if (current_selection >= cwd->folder.childCount + (cwd != root ? 1 : 0)) {
                            current_selection--;
                        }
                    } else {
                        // No items left, reset selection to 0
                        current_selection = 0;
                    }
                    
                    // Redraw the file explorer to reflect the changes
                    gui_draw_filesplorer();
                    return true;
                }
            }
            break;
            
        default:
            // Not a key we handle
            return false;
    }
    
    // If we get here, we changed the selection
    if (previous_selection != current_selection) {
        gui_draw_filesplorer(); // Redraw the whole UI
        
        return true; 

    }
}
/**
 * Handle keyboard input for the text editor
 */
bool gui_handle_editor_key(unsigned char key, char scancode) {
    if (!editor_active) return false;
    
    // Handle special keys
    switch (scancode) {
        case ESC_KEY_CODE:
            // Close editor and return to file explorer
            editor_active = false;
            explorer_active = true;
            gui_draw_filesplorer();
            return true;
            
        case ENTER_KEY_CODE:
            // Insert newline
            if (strlen(editor_content) < MAX_CONTENT - 1) {
                // Shift content to the right
                for (int i = strlen(editor_content); i >= editor_cursor_pos; i--) {
                    editor_content[i + 1] = editor_content[i];
                }
                editor_content[editor_cursor_pos] = '\n';
                editor_cursor_pos++;
                cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                editor_modified = true;
                
                // Auto-save
                filesys_editfile(editor_filename, editor_content);
                editor_modified = false; // Reset modified flag after saving
                
                gui_draw_text_editor();
            }
            return true;
            
        case BS_KEY_CODE:
            // Backspace - delete character before cursor
            if (editor_cursor_pos > 0) {
                // Shift content to the left
                for (int i = editor_cursor_pos; i <= strlen(editor_content); i++) {
                    editor_content[i - 1] = editor_content[i];
                }
                editor_cursor_pos--;
                cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                editor_modified = true;
                
                // Auto-save
                filesys_editfile(editor_filename, editor_content);
                editor_modified = false; // Reset modified flag after saving
                
                gui_draw_text_editor();
            }
            return true;
            
        case ARROW_UP_KEY:
            // Move cursor up one line
            if (editor_cursor_line > 0) {
                editor_cursor_line--;
                // Try to maintain column position
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                int line_length = get_line_length(line_start);
                if (editor_cursor_col > line_length) {
                    editor_cursor_col = line_length;
                }
                editor_cursor_pos = line_col_to_cursor_pos(editor_cursor_line, editor_cursor_col);
                gui_draw_text_editor();
            }
            return true;
            
        case ARROW_DOWN_KEY:
            // Move cursor down one line
            if (editor_cursor_line < count_lines(editor_content) - 1) {
                editor_cursor_line++;
                // Try to maintain column position
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                int line_length = get_line_length(line_start);
                if (editor_cursor_col > line_length) {
                    editor_cursor_col = line_length;
                }
                editor_cursor_pos = line_col_to_cursor_pos(editor_cursor_line, editor_cursor_col);
                gui_draw_text_editor();
            }
            return true;
            
        case ARROW_LEFT_KEY:
            // Move cursor left
            if (editor_cursor_pos > 0) {
                editor_cursor_pos--;
                cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                gui_draw_text_editor();
            }
            return true;
            
        case ARROW_RIGHT_KEY:
            // Move cursor right
            if (editor_cursor_pos < strlen(editor_content)) {
                editor_cursor_pos++;
                cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                gui_draw_text_editor();
            }
            return true;
            
        case 0x1F: // 'S' key - Show save status
            // Since auto-save is enabled, just show a confirmation
            return true;
            
        case 0x10: // 'Q' key - CTRL+Q to quit
            // Quit (auto-save has already saved changes)
            editor_active = false;
            explorer_active = true;
            gui_draw_filesplorer();
            return true;
            
        default:
            // Handle regular character input
            if (key >= 32 && key <= 126) { // Printable ASCII characters
                if (strlen(editor_content) < MAX_CONTENT - 1) {
                    // Check if we're at the end of a line that's already at max length
                    const char* line_start = get_line_start(editor_content, editor_cursor_line);
                    int line_length = get_line_length(line_start);
                    
                    // If the current line is at max length and we're at the end of it, 
                    // automatically wrap to the next line
                    if (line_length >= EDITOR_MAX_CHARS_PER_LINE && 
                        editor_cursor_col >= EDITOR_MAX_CHARS_PER_LINE) {
                        
                        // Insert a newline before adding the character
                        for (int i = strlen(editor_content); i >= editor_cursor_pos; i--) {
                            editor_content[i + 1] = editor_content[i];
                        }
                        editor_content[editor_cursor_pos] = '\n';
                        editor_cursor_pos++;
                        cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                    }
                    
                    // Now insert the character
                    for (int i = strlen(editor_content); i >= editor_cursor_pos; i--) {
                        editor_content[i + 1] = editor_content[i];
                    }
                    editor_content[editor_cursor_pos] = key;
                    editor_cursor_pos++;
                    cursor_pos_to_line_col(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                    editor_modified = true;
                    
                    // Auto-save
                    filesys_editfile(editor_filename, editor_content);
                    editor_modified = false; // Reset modified flag after saving
                    
                    gui_draw_text_editor();
                }
                return true;
            }
            break;
    }
    
    return false;
}




