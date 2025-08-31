#include "include/gui.h"

static uint8_t* vga_buffer = (uint8_t*)0xA0000;

int current_selection = 0;

char dialog_input[MAX_DIALOG_INPUT_LEN + 1] = "";
int dialog_input_pos = 0;
int dialog_type = 0;

char editor_content[MAX_CONTENT] = "";
char editor_filename[MAX_NAME_LEN] = "";
int editor_cursor_pos = 0;
int editor_scroll_line = 0;
int editor_cursor_line = 0;
int editor_cursor_col = 0;
bool editor_modified = false;

static int last_mouse_x = -1;
static int last_mouse_y = -1;

void gui_set_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_line_hrzt(int x1, int x2, int y, uint8_t color) {
    if (y < 0 || y >= SCREEN_HEIGHT) return;
    
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    if (x1 < 0) x1 = 0;
    if (x2 >= SCREEN_WIDTH) x2 = SCREEN_WIDTH - 1;
    
    for (int x = x1; x <= x2; x++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_line_vert(int x, int y1, int y2, uint8_t color) {
    if (x < 0 || x >= SCREEN_WIDTH) return;
    
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    if (y1 < 0) y1 = 0;
    if (y2 >= SCREEN_HEIGHT) y2 = SCREEN_HEIGHT - 1;
    
    for (int y = y1; y <= y2; y++) {
        vga_buffer[y * SCREEN_WIDTH + x] = color;
    }
}

void draw_rect(int x, int y, int width, int height, uint8_t color) {
    int x_end = x + width;
    int y_end = y + height;
    
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x_end > SCREEN_WIDTH) x_end = SCREEN_WIDTH;
    if (y_end > SCREEN_HEIGHT) y_end = SCREEN_HEIGHT;
    
    for (int j = y; j < y_end; j++) {
        for (int i = x; i < x_end; i++) {
            vga_buffer[j * SCREEN_WIDTH + i] = color;
        }
    }
}

void draw_rectoutline(int x, int y, int width, int height, uint8_t color) {
    draw_line_hrzt(x, x + width - 1, y, color);
    draw_line_hrzt(x, x + width - 1, y + height - 1, color);
    
    draw_line_vert(x, y, y + height - 1, color);
    draw_line_vert(x + width - 1, y, y + height - 1, color);
}

void draw_3dbox(int x, int y, int width, int height, 
                    uint8_t face_color, 
                    uint8_t highlight_color, 
                    uint8_t shadow_color) {
    draw_rect(x + 1, y + 1, width - 2, height - 2, face_color);
    
    draw_line_hrzt(x, x + width - 1, y, highlight_color);
    draw_line_vert(x, y, y + height - 1, highlight_color);
    
    draw_line_hrzt(x + 1, x + width - 1, y + height - 1, shadow_color);
    draw_line_vert(x + width - 1, y + 1, y + height - 1, shadow_color);
}

void draw_windowbox(int x, int y, int width, int height,
                        uint8_t outer_color,
                        uint8_t inner_color,
                        uint8_t face_color) {
    
    draw_rectoutline(x, y, width, height, outer_color);
    
    
    draw_rectoutline(x + 1, y + 1, width - 2, height - 2, inner_color);
    
    
    draw_rect(x + 2, y + 2, width - 4, height - 4, face_color);
}



void gui_clear(uint8_t color) {
    // clear screen
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        vga_buffer[i] = color;
    }
    
    
    gui_updatemouse();
}

void draw_title(int x, int y, int width, int title_height, uint8_t title_color) {
    
    draw_rect(x + 2, y + 2, width - 4, title_height, title_color);
    
    
    draw_line_hrzt(x + 2, x + width - 3, y + title_height + 1, VGA_COLOR_BLACK);
}


void gui_init() {
    
    outb(0x3C2, 0x63);
    
    
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    
    
    
    outb(0x3D4, 0x03); outb(0x3D5, inb(0x3D5) | 0x80);
    outb(0x3D4, 0x11); outb(0x3D5, inb(0x3D5) & ~0x80);
    
    
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
    
    
    static const uint8_t gc_reg[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
    static const uint8_t gc_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F, 0xFF};
    
    for (int i = 0; i < 9; i++) {
        outb(0x3CE, gc_reg[i]);
        outb(0x3CF, gc_data[i]);
    }
    
    
    
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
    
    
    outb(0x3C0, 0x20);
    
    
    vga_buffer = (uint8_t*)0xA0000;
}

void draw_char(int x, int y, char c, uint8_t color) {
    
    const uint8_t *glyph = system_font[(unsigned char)c];
    
    
    int char_width = char_widths[(unsigned char)c];
    
    
    int offset = 0;
    if (char_width < 8) {
        offset = (8 - char_width) / 2;
    }
    
    
    for (int row = 0; row < 8; row++) {
        uint8_t row_data = glyph[row];
        
        for (int col = 0; col < char_width; col++) {
            
            if (row_data & (0x80 >> (col + offset))) {
                gui_set_pixel(x + col, y + row, color);
            }
        }
    }
}

void draw_text(int x, int y, const char* text, uint8_t color) {
    int current_x = x;
    
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        
        
        if (c == '\n') {
            current_x = x;
            y += 9;  
            continue;
        }
        
        
        draw_char(current_x, y, c, color);
        
        
        current_x += char_widths[c] + 1;  
        
        
        if (current_x >= SCREEN_WIDTH - 8) {
            current_x = x;
            y += 9;  
        }
    }
}

int get_textwidth(const char* text) {
    int width = 0;
    
    for (int i = 0; text[i] != '\0'; i++) {
        unsigned char c = (unsigned char)text[i];
        width += char_widths[c] + 1;  
    }
    
    
    if (width > 0) width--;
    
    return width;
}

void draw_icon(int x, int y, const uint8_t icon[][16], int width, int height, uint8_t bg_color) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t pixel = icon[row][col];
            uint8_t color;
            
            if (pixel == 0) {
                
                color = bg_color;
            } else {
                color = icon_color_map[pixel];
            }
            
            gui_set_pixel(x + col, y + row, color);
        }
    }
}

void draw_file(int x, int y, const char* name, int is_dir, int is_selected) {
    
    int item_width = 60;
    int item_height = 40;
    int icon_width = 16;
    int icon_height = 16;
    
    
    int center_x = x + item_width/2;
    
    
    if (is_selected) {
        
        draw_rect(x, y, item_width, item_height, VGA_COLOR_BLUE);
    }
    
    
    int icon_x = center_x - (icon_width/2);
    int icon_y = y + 4;  
    
    
    if (is_dir) {
        draw_icon(icon_x, icon_y, folder_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY);
    } else {
        draw_icon(icon_x, icon_y, file_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY);
    }
    
    
    uint8_t text_color = is_selected ? VGA_COLOR_WHITE : VGA_COLOR_BLACK;
    uint8_t bg_color = is_selected ? VGA_COLOR_BLUE : VGA_COLOR_LIGHT_GREY;
    
    
    int text_y = icon_y + icon_height + 2;
    
    
    int name_width = get_textwidth(name);
    int max_name_width = item_width - 4; 
    
    if (name_width <= max_name_width) {
        
        int text_x = center_x - (name_width / 2);
        draw_text(text_x, text_y, name, text_color);
    } else {
        
        char truncated[64];
        int i = 0;
        int current_width = 0;
        
        
        while (name[i] != '\0' && 
               current_width + char_widths[(unsigned char)name[i]] < max_name_width - get_textwidth("..")) {
            truncated[i] = name[i];
            current_width += char_widths[(unsigned char)name[i]] + 1;
            i++;
        }
        
        
        truncated[i] = '.';
        truncated[i+1] = '.';
        truncated[i+2] = '\0';
        
        
        int text_x = center_x - (current_width + get_textwidth("..")) / 2;
        
        
        draw_rect(text_x - 1, text_y, max_name_width + 2, 10, bg_color);
        
        
        draw_text(text_x, text_y, truncated, text_color);
    }
}
void draw_text_scroll(int x, int y, const char* text, int max_width, uint8_t color, uint8_t bg_color) {
    
    int text_width = get_textwidth(text);
    
    
    if (text_width <= max_width) {
        draw_text(x, y, text, color);
        return;
    }
    draw_rect(x, y, max_width, 10, bg_color);
    int text_len = strlen(text);
    int i = 0;
    int current_width = text_width;
    int ellipsis_width = get_textwidth("...");
    while (i < text_len && current_width > max_width - ellipsis_width) {
        current_width -= (char_widths[(unsigned char)text[i]] + 1);
        i++;
    }
    draw_text(x, y, "...", color);
    draw_text(x + ellipsis_width, y, &text[i], color);
}


void draw_dialog(const char* title, const char* prompt) {
    
    int width = 200;
    int height = 80;
    
    
    int x = (SCREEN_WIDTH - width) / 2;
    int y = (SCREEN_HEIGHT - height) / 2;
    
    
    draw_rect(x + 4, y + 4, width, height, VGA_COLOR_DARK_GREY); 
    draw_windowbox(x, y, width, height,
                      VGA_COLOR_BLACK,
                      VGA_COLOR_WHITE,
                      VGA_COLOR_LIGHT_GREY);
    
    
    draw_title(x, y, width, 15, VGA_COLOR_BLUE);
    draw_text(x + 10, y + 4, title, VGA_COLOR_WHITE);
    
    
    draw_text(x + 10, y + 25, prompt, VGA_COLOR_BLACK);

    
    int input_box_width = width - 20;
    draw_rect(x + 10, y + 40, input_box_width, 14, VGA_COLOR_WHITE);
    draw_rectoutline(x + 10, y + 40, input_box_width, 14, VGA_COLOR_BLACK);
    
    
    int cursor_char_width = 0;
    for (int i = 0; i < dialog_input_pos && dialog_input[i] != '\0'; i++) {
        cursor_char_width += char_widths[(unsigned char)dialog_input[i]] + 1;
    }
    
    
    int text_width = get_textwidth(dialog_input);
    int max_visible_width = input_box_width - 4;  
    
    if (text_width <= max_visible_width) {
        
        
        
        draw_rect(x + 11, y + 41, input_box_width - 2, 12, VGA_COLOR_WHITE);
        
        draw_text(x + 12, y + 42, dialog_input, VGA_COLOR_BLACK);
        
        
        int cursor_x = x + 12 + cursor_char_width;
        draw_line_vert(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
    } else {
        
        static int scroll_offset = 0;
        
        
        int padding = 10;
        int min_cursor_x = padding;
        int max_cursor_x = max_visible_width - padding;
        
        
        if (cursor_char_width < scroll_offset + min_cursor_x) {
            
            scroll_offset = cursor_char_width - min_cursor_x;
            if (scroll_offset < 0) scroll_offset = 0;
        } else if (cursor_char_width > scroll_offset + max_cursor_x) {
            
            scroll_offset = cursor_char_width - max_cursor_x;
        }
        
        
        draw_rect(x + 11, y + 41, input_box_width - 2, 12, VGA_COLOR_WHITE);
        
        
        int draw_x = x + 12;
        int current_width = 0;
        
        for (int i = 0; dialog_input[i] != '\0'; i++) {
            
            int char_width = char_widths[(unsigned char)dialog_input[i]] + 1;
            
            
            if (current_width + char_width > scroll_offset) {
                
                int char_x = draw_x + current_width - scroll_offset;
                
                
                if (char_x < x + 12 + max_visible_width && char_x + char_width > x + 12) {
                    draw_char(char_x, y + 42, dialog_input[i], VGA_COLOR_BLACK);
                }
            }
            
            current_width += char_width;
            
            
            if (current_width - scroll_offset > max_visible_width) {
                break;
            }
        }
        
        
        int cursor_x = x + 12 + (cursor_char_width - scroll_offset);
        draw_line_vert(cursor_x, y + 42, y + 42 + 8, VGA_COLOR_BLACK);
    }
    
    
    draw_text(x + 10, y + 62, "ENTER: OK", VGA_COLOR_DARK_GREY);
    draw_text(x + width - 70, y + 62, "ESC: Cancel", VGA_COLOR_DARK_GREY);
    
    
}

char* get_file_content(const char* filename) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, filename)) {
            return child->file.content;
        }
    }
    return NULL;
}

int count_lines(const char* text) {
    int lines = 1;
    for (int i = 0; text[i]; i++) {
        if (text[i] == '\n') {
            lines++;
        }
    }
    return lines;
}

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
    return text; 
}

int len_line(const char* line_start) {
    // get line length
    int length = 0;
    while (line_start[length] && line_start[length] != '\n') {
        length++;
    }
    return length;
}

void cursorpos2linecol(int pos, int* line, int* col) {
    *line = 0;  // Reset to 0
    *col = 0;   // Reset to 0
    
    // Bounds check
    if (pos < 0) pos = 0;
    if (pos > strlen(editor_content)) pos = strlen(editor_content);
    
    for (int i = 0; i < pos && editor_content[i]; i++) {
        if (editor_content[i] == '\n') {
            (*line)++;
            *col = 0;  // Reset column on new line
        } else {
            (*col)++;
        }
    }
}

int linecol2cursorpos(int line, int col) {
    int pos = 0;
    int current_line = 0;
    
    // Bounds check
    if (line < 0) line = 0;
    if (col < 0) col = 0;
    
    // Find the start of the target line
    while (current_line < line && editor_content[pos]) {
        if (editor_content[pos] == '\n') {
            current_line++;
        }
        pos++;
    }
    
    // Move to the target column within the line
    int current_col = 0;
    while (current_col < col && editor_content[pos] && editor_content[pos] != '\n') {
        pos++;
        current_col++;
    }
    
    return pos;
}

// Cursor tracking structure
typedef struct {
    int x, y;
    uint8_t original_color;
    bool is_modified;
} cursor_pixel_t;

static cursor_pixel_t cursor_pixels[64];
static int num_cursor_pixels = 0;

void draw_mouse(int x, int y) {
    num_cursor_pixels = 0;
    
    for (int j = 0; j < 8; j++) {
        for (int i = 0; i < 8; i++) {
            int screen_x = x + i;
            int screen_y = y + j;
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && 
                screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                uint8_t pattern = cursor_icon[j][i];
                // Only draw and track non-transparent pixels
                if (pattern == 1 || pattern == 2) {
                    // Save original pixel
                    cursor_pixels[num_cursor_pixels].x = screen_x;
                    cursor_pixels[num_cursor_pixels].y = screen_y;
                    cursor_pixels[num_cursor_pixels].original_color = vga_buffer[screen_y * SCREEN_WIDTH + screen_x];
                    cursor_pixels[num_cursor_pixels].is_modified = true;
                    num_cursor_pixels++;
                    
                    // Draw cursor pixel
                    if (pattern == 1) {
                        vga_buffer[screen_y * SCREEN_WIDTH + screen_x] = VGA_COLOR_BLACK;
                    } else if (pattern == 2) {
                        vga_buffer[screen_y * SCREEN_WIDTH + screen_x] = VGA_COLOR_WHITE;
                    }
                }
            }
        }
    }
}

void restore_cursor_pixels(void) {
    for (int i = 0; i < num_cursor_pixels; i++) {
        if (cursor_pixels[i].is_modified) {
            vga_buffer[cursor_pixels[i].y * SCREEN_WIDTH + cursor_pixels[i].x] = cursor_pixels[i].original_color;
        }
    }
    num_cursor_pixels = 0;
}

void update_mouse(void) {
    
    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    
    
    extern bool editor_active;
    if (editor_active) {
        return;
    }
    
    mouse_state_t* mouse = get_mouse_state();
    if (!mouse) return;
    
    int cursor_x = (mouse->x_position * SCREEN_WIDTH) / 640;
    int cursor_y = (mouse->y_position * SCREEN_HEIGHT) / 480;
    
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_x > SCREEN_WIDTH - 8) cursor_x = SCREEN_WIDTH - 8;
    if (cursor_y < 0) cursor_y = 0;
    if (cursor_y > SCREEN_HEIGHT - 8) cursor_y = SCREEN_HEIGHT - 8;
    
    // only update if moved
    if (cursor_x != last_mouse_x || cursor_y != last_mouse_y) {
        
        if (last_mouse_x >= 0 && last_mouse_y >= 0) {
            restore_cursor_pixels();
        }
        
        // draw cursor at new pos
        draw_mouse(cursor_x, cursor_y);
        
        // update position
        last_mouse_x = cursor_x;
        last_mouse_y = cursor_y;
    }
}

void gui_clearmouse(void) {
    if (last_mouse_x >= 0 && last_mouse_y >= 0) {
        restore_cursor_pixels();
    }
    
    last_mouse_x = -1;
    last_mouse_y = -1;
}

void gui_updatemouse(void) {
    
    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    
    update_mouse();
}

void draw_cursor(void) {
    extern bool dialog_active;
    if (dialog_active) {
        return;
    }
    last_mouse_x = -1;
    last_mouse_y = -1;
    update_mouse();
}
