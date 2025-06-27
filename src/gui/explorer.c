#include "gui.h"
#include "editor.h"
#include "dock.h"

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
 * Handle keyboard input for the dialog box
 */
bool gui_handle_dialog_key(unsigned char key, char scancode) {
    if (!dialog_active) return false;
    
    // Handle special keys
    if (scancode == ENTER_KEY_CODE) {
        // User pressed Enter - handle based on dialog type
        dialog_active = false;
        
        if (dialog_type == DIALOG_TYPE_NEW_FILE) {
            // This is a new file dialog from dock
            dock_create_and_open_file();
        } else {
            // Regular directory/file creation dialog
            dialog_create_item();
            gui_draw_filesplorer();  // Redraw explorer after item creation
        }
        
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        return true;
    } 
    else if (scancode == ESC_KEY_CODE) {
        // User pressed Escape - cancel
        dialog_active = false;
        dialog_input[0] = '\0';
        dialog_input_pos = 0;
        
        if (dialog_type == DIALOG_TYPE_NEW_FILE) {
            // Return to dock if canceling new file dialog
            dock_return();
        } else {
            // Return to explorer for other dialogs
            gui_draw_filesplorer();
        }
        return true;
    }
    else if (scancode == BS_KEY_CODE) {
        // Handle backspace
        if (dialog_input_pos > 0) {
            dialog_input_pos--;
            dialog_input[dialog_input_pos] = '\0';
            gui_draw_dialog("New File", "Enter filename:"); // Redraw dialog
        }
        return true;
    }
    else if (key >= 32 && key < 127) {
        // Handle printable characters
        if (dialog_input_pos < 128) {
            dialog_input[dialog_input_pos] = key;
            dialog_input_pos++;
            dialog_input[dialog_input_pos] = '\0';
            gui_draw_dialog("New File", "Enter filename:"); // Redraw dialog
        }
        return true;
    }
    
    return false;
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

        case ESC_KEY_CODE:
            // Return to dock when ESC is pressed
            dock_return();
            return true;
            
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
            dock_return();
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
            
        default:
            // Handle regular character input
            if (key >= 32 && key <= 126) { // Printable ASCII characters
                if (strlen(editor_content) < MAX_CONTENT - 1) {
                    // Check current line length before adding character
                    const char* line_start = get_line_start(editor_content, editor_cursor_line);
                    int line_length = get_line_length(line_start);
                    
                    // Only allow typing if we haven't reached the max line length
                    if (line_length < EDITOR_MAX_CHARS_PER_LINE) {
                        // Insert the character
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
                    // If line is at max length, simply ignore the character (don't type it)
                }
                return true;
            }
            break;
    }
    
    return false;
}