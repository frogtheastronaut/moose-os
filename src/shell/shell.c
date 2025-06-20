#include "../kernel/include/tty.h"
#include "../filesys/file.h"
#include "../lib/lib.h"

// For splitting strings

char buffer[256];
char* shell_cmds[] = {};

void shell_prompt() {
	no_delete = filesys_pwdlen() + 3; // 3 for " > ", change if necessary
	filesys_pwd(false);
	terminal_writestring(" > ", false);
}

void shell_process_command(char *command) {
    char parts[MAX_PARTS][MAX_PART_LEN];
    int count = split_string(command, ' ', parts);
    int write = 0;
    for (int read = 0; read < count; read++) {
        if (parts[read][0] != '\0') {
            if (write != read) {
                copyStr(parts[write], parts[read]);
            }
            write++;
        }
    }
    count = write;
    if (count == 0 || parts[0][0] == '\0') {
        return;
    }

    if (strEqual(parts[0], "ls")) {
        filesys_ls();
    } else if (strEqual(parts[0], "pwd")) {
        filesys_pwd(true);
    } else if (strEqual(parts[0], "cd")) {
        if (count < 2) {
            terminal_writestring("Usage: cd <dir>", true);
        } else {
            filesys_cd(parts[1]);
        }
    } else if (strEqual(parts[0], "mkdir")) {
        if (count < 2) {
            terminal_writestring("Usage: mkdir <dir>", true);
        } else {
            int result = filesys_mkdir(parts[1]);
            if (result == 0) terminal_writestring("Directory created successfully.", true);
            else if (result == -1) terminal_writestring("Error: Directory limit reached.", true);
            else if (result == -2) terminal_writestring("Error: Invalid directory name.", true);
            else if (result == -3) terminal_writestring("Error: Directory already exists.", true);
            else terminal_writestring("Error: Unknown error.", true);
        }
    } else if (strEqual(parts[0], "rmdir")) {
        if (count < 2) {
            terminal_writestring("Usage: rmdir <dir>", true);
        } else {
            int result = filesys_rmdir(parts[1]);
            if (result == 0) terminal_writestring("Directory removed successfully.", true);
            else if (result == -2) terminal_writestring("Error: Directory not empty.", true);
            else terminal_writestring("Error: Directory not found.", true);
        }
    } else if (strEqual(parts[0], "rm")) {
        if (count < 2) {
            terminal_writestring("Usage: rm <file>", true);
        } else {
            int result = filesys_rm(parts[1]);
            if (result == 0) terminal_writestring("File removed successfully.", true);
            else terminal_writestring("Error: File not found.", true);
        }
    } else if (strEqual(parts[0], "cat")) {
        if (count < 2) {
            terminal_writestring("Usage: cat <file>", true);
        } else {
            filesys_cat(parts[1]);
        }
    } else if (strEqual(parts[0], "touch")) {
        if (count < 2) {
            terminal_writestring("Usage: touch <file>", true);
        } else {
            int result = filesys_mkfile(parts[1], "");
            if (result == 0) terminal_writestring("File created successfully.", true);
            else if (result == -1) terminal_writestring("Error: File limit reached.", true);
            else if (result == -2) terminal_writestring("Error: Invalid file name.", true);
            else if (result == -3) terminal_writestring("Error: Invalid file content.", true);
            else if (result == -4) terminal_writestring("Error: File already exists.", true);
            else terminal_writestring("Error: Unknown error.", true);
        }
    } else {
        terminal_writestring("Unknown command.", true);
    }
}