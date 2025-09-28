/*
    MooseOS GUI icons code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#include "gui/gui.h"
#include "elements/gfx.h"
#include "elements/gui_text.h"

/**
 * draw icons
 * @param x X coordinate of the icon
 * @param y Y coordinate of the icon
 * @param icon pointer to the icon bitmap
 * @param width width of the icon
 * @param height height of the icon
 * @param bg_colour background colour
 */
void draw_icon(int x, int y, const uint8_t icon[][16], int width, int height, uint8_t bg_colour) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            uint8_t pixel = icon[row][col];
            uint8_t colour;
            
            if (pixel == 0) {
                
                colour = bg_colour;
            } else {
                colour = icon_colour_map[pixel];
            }
            gui_set_pixel(x + col, y + row, colour);
        }
    }
}
/**
 * draw a file
 * @param x X coordinate of the file
 * @param y Y coordinate of the file
 * @param name pointer to the file name string
 * @param is_dir flag indicating if the file is a directory
 * @param is_selected flag indicating if the file is selected
 *
 * @note here, we're assuming folder and file icons are bitmaps and they exist as assumed
 */
void draw_file(int x, int y, const char* name, int is_dir, int is_selected) {
    int item_width = 60;
    int item_height = 40;
    int icon_width = 16;
    int icon_height = 16;
    int center_x = x + item_width/2;
    
    if (is_selected) {
        draw_rect(x, y, item_width, item_height, VGA_COLOUR_BLUE);
    }
    
    int icon_x = center_x - (icon_width/2);
    int icon_y = y + 4;  
    
    // draw directory or file icon
    if (is_dir) {
        draw_icon(icon_x, icon_y, folder_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOUR_BLUE : VGA_COLOUR_LIGHT_GREY);
    } else {
        draw_icon(icon_x, icon_y, file_icon, icon_width, icon_height, 
                     is_selected ? VGA_COLOUR_BLUE : VGA_COLOUR_LIGHT_GREY);
    }
    
    uint8_t text_colour = is_selected ? VGA_COLOUR_WHITE : VGA_COLOUR_BLACK;
    uint8_t bg_colour = is_selected ? VGA_COLOUR_BLUE : VGA_COLOUR_LIGHT_GREY;
    int text_y = icon_y + icon_height + 2;
    int name_width = draw_text_width(name);
    int max_name_width = item_width - 4; 
    
    if (name_width <= max_name_width) {
        int text_x = center_x - (name_width / 2);
        draw_text(text_x, text_y, name, text_colour);
    } else {
        // draw truncated name with ellipsis
        char truncated[64];
        int i = 0;
        int current_width = 0;
        while (name[i] != '\0' && 
               current_width + char_widths[(unsigned char)name[i]] < max_name_width - draw_text_width("..")) {
            truncated[i] = name[i];
            current_width += char_widths[(unsigned char)name[i]] + 1;
            i++;
        }
        truncated[i] = '.';
        truncated[i+1] = '.';
        truncated[i+2] = '\0';
        int text_x = center_x - (current_width + draw_text_width("..")) / 2;
        draw_rect(text_x - 1, text_y, max_name_width + 2, 10, bg_colour);
        draw_text(text_x, text_y, truncated, text_colour);
    }
}
