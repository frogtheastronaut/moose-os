/**
 * Execute a command
 * 
 * @todo: Add tokenization + other features commonly found in a shell.
 *        This is high priority.
 */

#include "../include/terminal_cmd.h"
#include "../../kernel/include/speaker.h"
#include "../../lib/include/stdlib.h"

void term_exec_cmd(const char* cmd) {
    // Strip whitespace
    cmd = strip_whitespace(cmd);
    
    // Add cmd to history
    char prompt_line[CHARS_PER_LINE + 1];
    msnprintf(prompt_line, sizeof(prompt_line), "%s# %s", get_cwd(), cmd); 
    terminal_add_wrapped_text(prompt_line, TERM_PROMPT_COLOR);
    
    if (strlen(cmd) == 0) {
        return; // empty
    }
    // help
    /**
     * @todo: Double check this
     */
    if (strEqual(cmd, "help")) {
        terminal_print("Welcome to the MooseOS Terminal");
        terminal_print("ls - List files");
        terminal_print("cd <dir> - Change directory");
        terminal_print("mkdir <name> - Create directory");
        terminal_print("touch <name> - Create file");
        terminal_print("cat <file> - Show file content");
        terminal_print("diskinfo - Show disk information");
        terminal_print("memstats - Show memory statistics");
        terminal_print("save - Save current filesystem to disk");
        terminal_print("load - Load filesystem from disk");
        terminal_print("systest - Run system tests (disk, paging, interrupts)");
        terminal_print("beep - Play system beep");
        terminal_print("help - Show this help");
        terminal_print("clear - Clear terminal");
    }
    // ls
    else if (strEqual(cmd, "ls")) {
        if (cwd->folder.childCount == 0 || !cwd->folder.children) {
            terminal_print("Directory is empty.");
        } else {
            for (int i = 0; i < cwd->folder.childCount; i++) {
                File* child = cwd->folder.children[i];
                if (child) {
                    char line[CHARS_PER_LINE + 1];
                    if (child->type == FOLDER_NODE) {
                        msnprintf(line, sizeof(line), "[DIR]  %s", child->name);
                    } else {
                        msnprintf(line, sizeof(line), "[FILE] %s", child->name);
                    }
                    terminal_print(line);
                }
            }
        }
    }
    // cd
    else if (cmd[0] == 'c' && cmd[1] == 'd' && cmd[2] == ' ') {
        const char* dirname = cmd + 3;
        if (strEqual(dirname, "..")) {
            if (cwd->parent) {
                cwd = cwd->parent;
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Changed to %s", get_cwd());
                terminal_print(line);
            } else {
                terminal_print_error("Already at root");
            }
        } else if (strEqual(dirname, "/")) {
            cwd = root;
            terminal_print("Changed to /");
        } else {
            bool found = false;
            if (cwd->folder.children) {
                for (int i = 0; i < cwd->folder.childCount; i++) {
                    File* child = cwd->folder.children[i];
                    if (child && child->type == FOLDER_NODE && strEqual(child->name, dirname)) {
                        cwd = child;
                        char line[CHARS_PER_LINE + 1];
                        msnprintf(line, sizeof(line), "Changed to %s", dirname);
                        terminal_print(line);
                        found = true;
                        break;
                    }
                }
            }
            if (!found) {
                terminal_print_error("Directory not found");
            }
        }
    }
    // mkdir
    else if (cmd[0] == 'm' && cmd[1] == 'k' && cmd[2] == 'd' && cmd[3] == 'i' && cmd[4] == 'r' && cmd[5] == ' ') {
        const char* dirname = cmd + 6;
        if (strlen(dirname) > 0) {
            fs_make_dir(dirname);
            char line[CHARS_PER_LINE + 1];
            msnprintf(line, sizeof(line), "Created directory %s", dirname);
            terminal_print(line);
        } else {
            terminal_print_error("Usage: mkdir <name>");
        }
    }
    // touch
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' && cmd[4] == 'h' && cmd[5] == ' ') {
        const char* filename = cmd + 6;
        if (strlen(filename) > 0) {
            fs_make_file(filename, "");
            char line[CHARS_PER_LINE + 1];
            msnprintf(line, sizeof(line), "Created file %s", filename);
            terminal_print(line);
        } else {
            terminal_print_error("Usage: touch <name>");
        }
    }
    // cat
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
        const char* filename = cmd + 4;
        
        // Search for the file in current directory
        bool found = false;
        if (cwd->folder.children) {
            for (int i = 0; i < cwd->folder.childCount; i++) {
                File* child = cwd->folder.children[i];
                if (child && child->type == FILE_NODE && strEqual(child->name, filename)) {
                    found = true;
                    if (child->file.content && child->file.content_size > 0) {
                        // Print content directly - wrapping will be handled automatically
                        terminal_print(child->file.content);
                    } else {
                        terminal_print("File is empty");
                    }
                    break;
                }
            }
        }
        if (!found) {
            terminal_print_error("File not found");
        }
    }
    // clear term
    else if (strEqual(cmd, "clear")) {
        clear_terminal();
    }
    // show time/date
    else if (strEqual(cmd, "time")) {
        rtc_time local_time = rtc_gettime();
        char time_line[CHARS_PER_LINE + 1];
        char date_line[CHARS_PER_LINE + 1];
        
        char hour_str[3], min_str[3], sec_str[3];
        if (local_time.hours < 10) {
            hour_str[0] = '0';
            hour_str[1] = '0' + local_time.hours;
        } else {
            hour_str[0] = '0' + (local_time.hours / 10);
            hour_str[1] = '0' + (local_time.hours % 10);
        }
        hour_str[2] = '\0';
        
        if (local_time.minutes < 10) {
            min_str[0] = '0';
            min_str[1] = '0' + local_time.minutes;
        } else {
            min_str[0] = '0' + (local_time.minutes / 10);
            min_str[1] = '0' + (local_time.minutes % 10);
        }
        min_str[2] = '\0';
        
        if (local_time.seconds < 10) {
            sec_str[0] = '0';
            sec_str[1] = '0' + local_time.seconds;
        } else {
            sec_str[0] = '0' + (local_time.seconds / 10);
            sec_str[1] = '0' + (local_time.seconds % 10);
        }
        sec_str[2] = '\0';
        
        // Format date (DD/MM/YYYY)
        char month_str[3], day_str[3], year_str[5];
        if (local_time.month < 10) {
            month_str[0] = '0';
            month_str[1] = '0' + local_time.month;
        } else {
            month_str[0] = '0' + (local_time.month / 10);
            month_str[1] = '0' + (local_time.month % 10);
        }
        month_str[2] = '\0';
        
        if (local_time.day < 10) {
            day_str[0] = '0';
            day_str[1] = '0' + local_time.day;
        } else {
            day_str[0] = '0' + (local_time.day / 10);
            day_str[1] = '0' + (local_time.day % 10);
        }
        day_str[2] = '\0';

        // Year
        year_str[0] = '2';
        year_str[1] = '0';
        if (local_time.year < 10) {
            year_str[2] = '0';
            year_str[3] = '0' + local_time.year;
        } else {
            year_str[2] = '0' + (local_time.year / 10);
            year_str[3] = '0' + (local_time.year % 10);
        }
        year_str[4] = '\0';
        
        msnprintf(time_line, sizeof(time_line), "Time: %s:%s:%s", hour_str, min_str, sec_str);
        terminal_print(time_line);
        msnprintf(date_line, sizeof(date_line), "Date: %s/%s/%s", day_str, month_str, year_str);
        terminal_print(date_line);
    }
    // settimezone - set UTC+X
    else if (cmd[0] == 's' && cmd[1] == 'e' && cmd[2] == 't' && cmd[3] == 't' && cmd[4] == 'i' && 
             cmd[5] == 'm' && cmd[6] == 'e' && cmd[7] == 'z' && cmd[8] == 'o' && cmd[9] == 'n' && 
             cmd[10] == 'e' && cmd[11] == ' ') {
        const char* offset_str = cmd + 12;
        if (strlen(offset_str) > 0) {
            int offset = 0;
            int sign = 1;
            int i = 0;
            
            if (offset_str[0] == '-') {
                sign = -1;
                i = 1;
            } else if (offset_str[0] == '+') {
                i = 1;
            }
            
            while (offset_str[i] >= '0' && offset_str[i] <= '9') {
                offset = offset * 10 + (offset_str[i] - '0');
                i++;
            }
            
            offset *= sign;
            
            if (offset >= -12 && offset <= 14) {
                timezone_offset = offset; 
                terminal_print("Timezone updated");
            } else {
                timezone_offset = offset;
                terminal_print_error("Wierd timezone, but sure");
            }
        } else {
            terminal_print_error("Usage: settimezone <hours>");
        }
    }
    
    else if (strEqual(cmd, "diskinfo")) {
        char info_buffer[512];
        if (fs_get_disk_info(info_buffer, sizeof(info_buffer)) == 0) {
            char *line_start = info_buffer;
            char *line_end;
            
            while (*line_start) {
                line_end = line_start;
                // Find end of line or end of string
                while (*line_end && *line_end != '\n') {
                    line_end++;
                }
                
                // Create null-terminated line
                char temp_char = *line_end;
                *line_end = '\0';
                terminal_print(line_start);
                *line_end = temp_char;
                
                // Move to next line
                if (*line_end == '\n') {
                    line_end++;
                }
                line_start = line_end;
            }
        } else {
            terminal_print_error("Failed to get disk information");
        }
    }
    
    else if (strEqual(cmd, "memstats")) {
        char stats_buffer[512];
        if (filesys_get_memory_stats(stats_buffer, sizeof(stats_buffer)) == 0) {
            char *line_start = stats_buffer;
            char *line_end;
            
            while (*line_start) {
                line_end = line_start;
                // Find end of line or end of string
                while (*line_end && *line_end != '\n') {
                    line_end++;
                }
                
                // Create null-terminated line
                char temp_char = *line_end;
                *line_end = '\0';
                terminal_print(line_start);
                *line_end = temp_char;
                
                // Move to next line
                if (*line_end == '\n') {
                    line_end++;
                }
                line_start = line_end;
            }
        } else {
            terminal_print_error("Failed to get memory statistics");
        }
    }

    // save - save current filesystem to disk
    else if (strEqual(cmd, "save")) {
        if (filesys_disk_status()) {
            terminal_print("Saving filesystem to disk...");
            
            // First try to sync
            int sync_result = filesys_sync();
            if (sync_result != 0) {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Sync failed with error: %d", sync_result);
                terminal_print_error(line);
            } else {
                terminal_print("Superblock synced successfully");
            }
            
            // Then save filesystem data
            int save_result = fs_save_to_disk();
            if (save_result == 0) {
                terminal_print("Filesystem data saved successfully");
                
                // Force cache flush
                filesys_flush_cache();
                terminal_print("Cache flushed to disk");
                
                terminal_print("=== SAVE COMPLETE ===");
            } else {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Save failed with error: %d", save_result);
                terminal_print_error(line);
            }
        } else {
            terminal_print_error("No filesystem mounted");
        }
    }
    
    // load - load filesystem from disk
    else if (strEqual(cmd, "load")) {
        if (filesys_disk_status()) {
            if (fs_load_from_disk() == 0) {
                terminal_print("Filesystem loaded from disk successfully");
            } else {
                terminal_print_error("Failed to load filesystem from disk");
            }
        } else {
            terminal_print_error("No filesystem mounted");
        }
    }
    
    // systest - system test (compact version)
    else if (strEqual(cmd, "systest")) {
        terminal_print("=== System Test ===");
        
        char line[CHARS_PER_LINE + 1];
        
        // Test interrupts
        uint32_t eflags;
        asm volatile("pushf; pop %0" : "=r"(eflags));
        if (eflags & 0x200) {
            terminal_print("[PASS] Interrupts enabled");
        } else {
            terminal_print("[FAIL] Interrupts disabled");
        }
        
        // Test paging - show real-time activity
        uint32_t cr0;
        asm volatile("mov %%cr0, %0" : "=r"(cr0));
        if (cr0 & 0x80000000) {
            // Get current page directory
            uint32_t cr3;
            asm volatile("mov %%cr3, %0" : "=r"(cr3));
            msnprintf(line, sizeof(line), "[PASS] Paging active @0x%08X", cr3);
            terminal_print(line);
            
            // Show live memory translation
            uint32_t virt_addr = 0x100000; // 1MB mark
            uint32_t *page_dir = (uint32_t*)(cr3 & 0xFFFFF000);
            uint32_t pde_index = virt_addr >> 22;
            uint32_t pte_index = (virt_addr >> 12) & 0x3FF;
            
            msnprintf(line, sizeof(line), "[INFO] Virt 0x%06X -> PDE[%u] PTE[%u]", 
                virt_addr, pde_index, pte_index);
            terminal_print(line);
        } else {
            terminal_print("[FAIL] Paging disabled");
        }
        
        if (cr0 & 0x1) {
            terminal_print("[PASS] Protected mode");
        } else {
            terminal_print("[FAIL] Real mode");
        }
        
        // Test memory
        uint32_t *test_addr = (uint32_t*)0x100000;
        uint32_t orig = *test_addr;
        *test_addr = 0xDEADBEEF;
        if (*test_addr == 0xDEADBEEF) {
            terminal_print("[PASS] Memory access OK");
            *test_addr = orig;
        } else {
            terminal_print("[FAIL] Memory access failed");
        }
        
        // Test disk
        extern ata_device_t ata_devices[4];
        if (ata_devices[0].exists) {
            terminal_print("[PASS] Disk detected");
            
            uint8_t buffer[512];
            for (int i = 0; i < 512; i++) buffer[i] = i % 256;
            
            if (disk_write_sector(0, 100, buffer) == 0) {
                terminal_print("[PASS] Disk write OK");
                
                for (int i = 0; i < 512; i++) buffer[i] = 0xCC;
                
                if (disk_read_sector(0, 100, buffer) == 0) {
                    if (buffer[0] == 0 && buffer[1] == 1) {
                        terminal_print("[PASS] Disk read OK");
                    } else {
                        terminal_print("[FAIL] Data corruption");
                    }
                } else {
                    terminal_print("[FAIL] Disk read failed");
                }
            } else {
                terminal_print("[FAIL] Disk write failed");
            }
        } else {
            terminal_print("[FAIL] No disk found");
        }
        
        // Summary
        terminal_print("=== Test Complete ===");
    }
    
    // Audio commands
    else if (strEqual(cmd, "beep")) {
        terminal_print("Playing system beep...");
        speaker_system_beep();
        terminal_print("Beep complete");
    }
    
    // tone <frequency> - Play custom frequency
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'n' && cmd[3] == 'e' && cmd[4] == ' ') {
        const char* freq_str = cmd + 5;
        if (strlen(freq_str) > 0) {
            uint32_t frequency = atoi(freq_str);
            if (frequency >= SPEAKER_MIN_FREQ && frequency <= SPEAKER_MAX_FREQ) {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Playing %u Hz for 1 second...", frequency);
                terminal_print(line);
                speaker_beep(frequency, 1000);
                terminal_print("Tone complete");
            } else {
                char line[CHARS_PER_LINE + 1];
                msnprintf(line, sizeof(line), "Frequency must be between %u and %u Hz", 
                         SPEAKER_MIN_FREQ, SPEAKER_MAX_FREQ);
                terminal_print_error(line);
            }
        } else {
            terminal_print_error("Usage: tone <frequency>");
        }
    }
    
    // Unknown command
    else {
        char line[CHARS_PER_LINE + 1];
        msnprintf(line, sizeof(line), "Unknown command: %s", cmd);
        terminal_print_error(line);
        terminal_print("Type 'help' for commands");
    }
}