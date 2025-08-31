/**
 * Moose Operating System
 * Copyright (c) 2025 Ethan Zhang.
 * 
 * @todo: If we ever make it so we can change the folder/file icon size, 
 *        we would need to update this code. We wouldn't be considering this as an immediate priority.
 */

#include "include/explorer.h"

/**
 * Draw file explorer
 */
void draw_explorer() {
    gui_init();
    gui_clear(VGA_COLOR_LIGHT_GREY);
    
    draw_windowbox(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 
                       VGA_COLOR_LIGHT_GREY,
                       VGA_COLOR_LIGHT_GREY,
                       VGA_COLOR_LIGHT_GREY);
    
    // Title bar
    draw_title(0, 0, SCREEN_WIDTH, 20, VGA_COLOR_BLUE);
    char path_text[64] = "File Explorer - ";
    draw_text(10, 6, path_text, VGA_COLOR_WHITE);

    // Display cwd
    char full_path[128] = "";
    if (cwd == root) {
        copyStr(full_path, "/");
    } else {
        File* node = cwd;
        
        copyStr(full_path, node->name);
        node = node->parent;
        
        while (node != NULL && node != root) {
            char temp[128] = "";
            copyStr(temp, node->name);
            strcat(temp, "/");
            strcat(temp, full_path);
            copyStr(full_path, temp);
            
            node = node->parent;
        }
        
        char temp[128] = "/";
        strcat(temp, full_path);
        copyStr(full_path, temp);
    }
    
    int path_x = 10 + get_textwidth(path_text);
    int available_width = SCREEN_WIDTH - path_x - 10; 
    draw_text_scroll(path_x, 6, full_path, available_width, VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    
    int x_pos = 20;
    int y_pos = 30;
    int displayed_count = 0; 
    
    if (cwd != root) {
        draw_file(x_pos, y_pos, "..", 1, current_selection == 0);
        x_pos += 70;
        displayed_count++;
        
        if (displayed_count % 4 == 0) { // 4 items per row
            x_pos = 20;
            y_pos += 40;
        }
    }
    
    // draw files and folders
    for (int i = 0; i < cwd->folder.childCount && displayed_count < 16; i++) { // 4 rows of 4 items each
        File* child = cwd->folder.children[i];

        int selection_index = i;
        if (cwd != root) {
            selection_index = i + 1;  // +1 bc of ".." folder 
        }
        
        draw_file(x_pos, y_pos, child->name, 
                        (child->type == FOLDER_NODE), 
                        current_selection == selection_index);

        x_pos += 70;
        displayed_count++;

        if (displayed_count % 4 == 0) { // 4 items per row
            x_pos = 20;
            y_pos += 40;
        }
    }

    // Item count
    char count_str[16]; 
    int2str(cwd->folder.childCount, count_str, sizeof(count_str));
    
    char status_text[32] = "";
    copyStr(status_text, count_str);
    
    if (cwd->folder.childCount == 1) {
        strcat(status_text, " item");
    } else {
        strcat(status_text, " items");
    }
    
    draw_text(15, SCREEN_HEIGHT - 15, status_text, VGA_COLOR_BLACK);
    explorer_active = true;
    
    draw_cursor();
}
/**
 * Create an item
 */
void dialog_create_item() {
    if (dialog_input[0] != '\0') {
        int previous_item_count = cwd->folder.childCount;
        
        if (dialog_type == 0) {
            // Create directory
            filesys_mkdir(dialog_input);
        } else {
            // Create file
            filesys_mkfile(dialog_input, "");
        }
        
        if (cwd->folder.childCount > previous_item_count) {
            if (cwd != root) {
                current_selection = (cwd->folder.childCount - 1) + 1; // +1 bc of ".." folder
            } else {
                current_selection = cwd->folder.childCount - 1;
            }
            
            int max_visible = 12;
            if (cwd != root) {
                max_visible = 11;
            }
            
            if (current_selection >= max_visible) {
                current_selection = 0;
            }
        } else {
            // Reset if item creation failed/no new item created
            current_selection = 0;
        }
        draw_explorer();
    }
}
/**
 * Handle keyboard input for dialog
 */
bool gui_handle_dialog_input(unsigned char key, char scancode) {
    if (!dialog_active) return false;
    
    if (scancode == ENTER_KEY_CODE) {
        dialog_active = false;
        
        if (dialog_type == DIALOG_TYPE_NEW_FILE) {
            dock_mkopen_file();
        } else {
            dialog_create_item();
            draw_explorer();  // Redraw
        }
        
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        return true;
    } 
    else if (scancode == ESC_KEY_CODE) {
        dialog_active = false;
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        
        if (dialog_type == DIALOG_TYPE_NEW_FILE) {

            dock_return();
        } else {
            draw_explorer();
        }
        return true;
    }
    else if (scancode == ARROW_LEFT_KEY) {
        if (dialog_input_pos > 0) {
            dialog_input_pos--;
            if (dialog_type == 0) {
                draw_dialog("Create Directory", "Enter directory name:");
            } else if (dialog_type == 1) {
                draw_dialog("Create File", "Enter file name:");
            } else {
                draw_dialog("New File", "Enter filename:");
            }
            
            draw_cursor();
        }
        return true;
    }
    else if (scancode == ARROW_RIGHT_KEY) {
        if (dialog_input_pos < strlen(dialog_input)) {
            dialog_input_pos++;
            if (dialog_type == 0) {
                draw_dialog("Create Directory", "Enter directory name:");
            } else if (dialog_type == 1) {
                draw_dialog("Create File", "Enter file name:");
            } else {
                draw_dialog("New File", "Enter filename:");
            }
            
            draw_cursor();
        }
        return true;
    }
    else if (scancode == BS_KEY_CODE) {
        if (dialog_input_pos > 0) {
            for (int i = dialog_input_pos - 1; i < strlen(dialog_input); i++) {
                dialog_input[i] = dialog_input[i + 1];
            }
            dialog_input_pos--;
            if (dialog_type == 0) {
                draw_dialog("Create Directory", "Enter directory name:");
            } else if (dialog_type == 1) {
                draw_dialog("Create File", "Enter file name:");
            } else {
                draw_dialog("New File", "Enter filename:");
            }
            
            draw_cursor();
        }
        return true;
    }
    else if (key >= 32 && key < 127) {
        if (strlen(dialog_input) < 128) {
            for (int i = strlen(dialog_input); i >= dialog_input_pos; i--) {
                dialog_input[i + 1] = dialog_input[i];
            }
            dialog_input[dialog_input_pos] = key;
            dialog_input_pos++;
            if (dialog_type == 0) {
                draw_dialog("Create Directory", "Enter directory name:");
            } else if (dialog_type == 1) {
                draw_dialog("Create File", "Enter file name:");
            } else {
                draw_dialog("New File", "Enter filename:");
            }
            
            draw_cursor();
        }
        return true;
    }
    
    return false;
}
/*
 * Handle explorer key inputs
 */
bool gui_handle_explorer_key(unsigned char key, char scancode) {
    if (dialog_active) {
        return gui_handle_dialog_input(key, scancode);
    }
    
    if (!explorer_active) return false;
    
    int items_per_row = 4;
    int total_items = cwd->folder.childCount;
    if (cwd != root) total_items++; // Account for ".." folder
    
    // Update previous selection
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
            if (cwd != root && current_selection == 0) {
                filesys_cd("..");
                current_selection = 0;
                draw_explorer();
                return true;
            } else {
                int actual_index = current_selection;
                if (cwd != root) actual_index -= 1;  // account for ".." entry
                
                if (actual_index >= 0 && actual_index < cwd->folder.childCount) {
                    File* child = cwd->folder.children[actual_index];
                    if (child->type == FOLDER_NODE) {
                        filesys_cd(child->name);
                        current_selection = 0;
                        draw_explorer();
                        return true;
                    } else {
                        editor_open(child->name);
                        return true;
                    }
                }
            }
            break;
            
        case 0x20:
            dialog_active = true;
            dialog_type = 0; 
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            draw_dialog("Create Directory", "Enter directory name:");
            
            draw_cursor();
            return true;

        case 0x21:
            dialog_active = true;
            dialog_type = 1; // File
            dialog_input[0] = '\0';
            dialog_input_pos = 0;
            draw_dialog("Create File", "Enter file name:");
            draw_cursor();
            return true;

        case BS_KEY_CODE:
            if (cwd != root && current_selection == 0) {
                // can't delete the ".." entry
                return true;
            } else {
                int actual_index = current_selection;
                if (cwd != root) actual_index -= 1;  // account for ".." entry

                if (actual_index >= 0 && actual_index < cwd->folder.childCount) {
                    File* child = cwd->folder.children[actual_index];
                    
                    if (child->type == FOLDER_NODE) {
                        // Delete folder
                        filesys_rmdir(child->name);
                    } else {
                        // Delete file
                        filesys_rm(child->name);
                    }
                    
                    if (cwd->folder.childCount > 0) {
                        if (current_selection >= cwd->folder.childCount + (cwd != root ? 1 : 0)) {
                            current_selection--;
                        }
                    } else {
                        current_selection = 0;
                    }
                    
                    draw_explorer();
                    return true;
                }
            }
            break;

        case ESC_KEY_CODE:
            // Return to dock
            dock_return();
            return true;
            
        default:
            // Not a handleable key
            return false;
    }
    
    if (previous_selection != current_selection) {
        draw_explorer();
        
        return true; 

    }
}

/**
 * Handle editor input
 */
bool gui_handle_editor_key(unsigned char key, char scancode) {
    if (!editor_active) return false;
    
    switch (scancode) {
        case ESC_KEY_CODE:
            editor_active = false;
            explorer_active = true;
            dock_return();
            extern void gui_updatemouse(void);
            gui_updatemouse();
            return true;
            
        case ENTER_KEY_CODE:
            if (strlen(editor_content) < MAX_CONTENT - 1) {
                for (int i = strlen(editor_content); i >= editor_cursor_pos; i--) {
                    editor_content[i + 1] = editor_content[i];
                }
                editor_content[editor_cursor_pos] = '\n';
                editor_cursor_pos++;
                cursorpos2linecol(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                if (editor_cursor_line >= editor_scroll_line + EDITOR_LINES_VISIBLE) {
                    editor_scroll_line = editor_cursor_line - EDITOR_LINES_VISIBLE + 1;
                }
                editor_modified = true;
                filesys_editfile(editor_filename, editor_content);
                editor_modified = false;
                editor_draw();
            }
            return true;
            
        case BS_KEY_CODE:
            if (editor_cursor_pos > 0) {
                for (int i = editor_cursor_pos; i <= strlen(editor_content); i++) {
                    editor_content[i - 1] = editor_content[i];
                }
                editor_cursor_pos--;
                cursorpos2linecol(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                if (editor_cursor_line < editor_scroll_line) {
                    editor_scroll_line = editor_cursor_line;
                }
                editor_modified = true;
                filesys_editfile(editor_filename, editor_content);
                editor_modified = false;
                editor_draw();
            }
            return true;
            
        case ARROW_UP_KEY:
            if (editor_cursor_line > 0) {
                editor_cursor_line--;
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                int line_length = len_line(line_start);
                if (editor_cursor_col > line_length) {
                    editor_cursor_col = line_length;
                }
                editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                if (editor_cursor_line < editor_scroll_line) {
                    editor_scroll_line = editor_cursor_line;
                }
                editor_draw();
            }
            return true;
            
        case ARROW_DOWN_KEY:
            if (editor_cursor_line < count_lines(editor_content) - 1) {
                editor_cursor_line++;
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                int line_length = len_line(line_start);
                if (editor_cursor_col > line_length) {
                    editor_cursor_col = line_length;
                }
                editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                if (editor_cursor_line >= editor_scroll_line + EDITOR_LINES_VISIBLE) {
                    editor_scroll_line = editor_cursor_line - EDITOR_LINES_VISIBLE + 1;
                }
                editor_draw();
            }
            return true;
            
        case ARROW_LEFT_KEY:
            if (editor_cursor_col > 0) {
                editor_cursor_col--;
                editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                editor_draw();
            } else if (editor_cursor_line > 0) {
                editor_cursor_line--;
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                editor_cursor_col = len_line(line_start);
                editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                if (editor_cursor_line < editor_scroll_line) {
                    editor_scroll_line = editor_cursor_line;
                }
                editor_draw();
            }
            return true;
            
        case ARROW_RIGHT_KEY:
            {
                const char* line_start = get_line_start(editor_content, editor_cursor_line);
                int line_length = len_line(line_start);
                if (editor_cursor_col < line_length) {
                    editor_cursor_col++;
                    editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                    editor_draw();
                } else if (editor_cursor_line < count_lines(editor_content) - 1) {
                    editor_cursor_line++;
                    editor_cursor_col = 0;
                    editor_cursor_pos = linecol2cursorpos(editor_cursor_line, editor_cursor_col);
                    if (editor_cursor_line >= editor_scroll_line + EDITOR_LINES_VISIBLE) {
                        editor_scroll_line = editor_cursor_line - EDITOR_LINES_VISIBLE + 1;
                    }
                    editor_draw();
                }
            }
            return true;
            
        default:
            if (key >= 32 && key <= 126) {
                if (strlen(editor_content) < MAX_CONTENT - 1) {
                    for (int i = strlen(editor_content); i >= editor_cursor_pos; i--) {
                        editor_content[i + 1] = editor_content[i];
                    }
                    editor_content[editor_cursor_pos] = key;
                    editor_cursor_pos++;
                    cursorpos2linecol(editor_cursor_pos, &editor_cursor_line, &editor_cursor_col);
                    if (editor_cursor_line >= editor_scroll_line + EDITOR_LINES_VISIBLE) {
                        editor_scroll_line = editor_cursor_line - EDITOR_LINES_VISIBLE + 1;
                    }
                    editor_modified = true;
                    filesys_editfile(editor_filename, editor_content);
                    editor_modified = false;
                    editor_draw();
                }
                return true;
            }
            break;
    }
    
    return false;
}

/**
 * Handle mouse clicks in the explorer
 */
static bool explorer_handle_click(int mouse_x, int mouse_y) {
    if (!explorer_active || dialog_active) {
        return false;
    }
    int start_x = 30;
    int start_y = 40;
    int item_spacing_x = 70;  
    int item_spacing_y = 40;
    int item_width = 60;
    int item_height = 40;
    int items_per_row = 4;
    

    int total_items = cwd->folder.childCount;
    if (cwd != root) total_items++;
    

    if (mouse_x < start_x || mouse_y < start_y) {
        return false;
    }
    
    int click_col = (mouse_x - start_x) / item_spacing_x;
    int click_row = (mouse_y - start_y) / item_spacing_y;
    
    if (click_col >= items_per_row) {
        return false;
    }
    
    int clicked_selection = click_row * items_per_row + click_col;
    
    if (clicked_selection >= total_items) {
        return false;
    }
    
    int item_x = start_x + (click_col * item_spacing_x);
    int item_y = start_y + (click_row * item_spacing_y);
    
    if (mouse_x >= item_x && mouse_x < item_x + item_width &&
        mouse_y >= item_y && mouse_y < item_y + item_height) {
        
        int previous_selection = current_selection;
        current_selection = clicked_selection;
        
        if (previous_selection != current_selection) {
            draw_explorer();
        }
        
        return true;
    }
    
    return false;
}

/**
 * Handle mouse input for file explorer
 */
bool explorer_handle_mouse() {
    static bool last_left_state = false;
    
    if (!explorer_active) {
        return false;
    }
    
    mouse_state_t* mouse = get_mouse_state();
    if (!mouse) {
        return false;
    }
    

    int mouse_x = (mouse->x_position * SCREEN_WIDTH) / 640;
    int mouse_y = (mouse->y_position * SCREEN_HEIGHT) / 480;
    
    if (mouse->left_button) {
        if (!last_left_state) {
            last_left_state = true;
            return explorer_handle_click(mouse_x, mouse_y);
        }
    } else {
        last_left_state = false;
    }
    
    return false;
}