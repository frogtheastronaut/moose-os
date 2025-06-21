#include "../kernel/include/tty.h"
#include "../filesys/file.h"
#include "../lib/lib.h"

// For splitting strings
#define VERSION "0.0.5"
#define SYS "simpleMOS"
char name[32] = "root";
char buffer[256];
char* shell_cmds[] = {};

void shell_prompt() {
	no_delete = strlen(SYS) + strlen(name) + filesys_pwdlen() + 4; // "root@/# ", change if necessary
    terminal_writestring(name, false);
    terminal_writestring("@", false);
    terminal_writestring(SYS, false);
	filesys_pwd(false);
	terminal_writestring(" # ", false);
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
            terminal_writestring("Error: CD NEEDS ARGUMENTS", true);
        } else {
            filesys_cd(parts[1]);
        }
    } else if (strEqual(parts[0], "mkdir")) {
        if (count < 2) {
            terminal_writestring("Error: MKDIR NEEDS ARGUMENTS", true);
        } else {
            int result = filesys_mkdir(parts[1]);
            if (result == 0) terminal_writestring("MKDIR SUCCESSFUL", true);
            else if (result == -1) terminal_writestring("Error: DIR LIMIT REACHED", true);
            else if (result == -2) terminal_writestring("Error: DIR NAME INVALID", true);
            else if (result == -3) terminal_writestring("Error: DIR ALREADY EXISTS", true);
            else terminal_writestring("ERROR", true);
        }
    } else if (strEqual(parts[0], "rmdir")) {
        if (count < 2) {
            terminal_writestring("Error: RMDIR NEEDS ARGUMENTS", true);
        } else {
            int result = filesys_rmdir(parts[1]);
            if (result == 0) terminal_writestring("RMDIR SUCCESS", true);
            else if (result == -2) terminal_writestring("Error: DIR NOT EMPTY", true);
            else terminal_writestring("Error: DIR NOT FOUND", true);
        }
    } else if (strEqual(parts[0], "rm")) {
        if (count < 2) {
            terminal_writestring("Error: RM NEEDS ARGUMENTS", true);
        } else {
            int result = filesys_rm(parts[1]);
            if (result == 0) terminal_writestring("RM SUCCESS", true);
            else terminal_writestring("Error: FILE NOT FOUND", true);
        }
    } else if (strEqual(parts[0], "cat")) {
        if (count < 2) {
            terminal_writestring("Error: CAT NEEDS ARGUMENTS", true);
        } else {
            filesys_cat(parts[1]);
        }
    } else if (strEqual(parts[0], "touch")) {
        if (count < 2) {
            terminal_writestring("Error: TOUCH NEEDS ARGUMENTS", true);
        } else {
            int result = filesys_mkfile(parts[1], "");
            if (result == 0) terminal_writestring("TOUCH SUCCESS", true);
            else if (result == -1) terminal_writestring("Error: FILE LIMIT REACHED", true);
            else if (result == -2) terminal_writestring("Error: INVALID FILE NAME", true);
            else if (result == -3) terminal_writestring("Error: INVALID FILE CONTENT", true);
            else if (result == -4) terminal_writestring("Error: FILE ALREADY EXISTS.", true);
            else terminal_writestring("ERROR", true);
        }
    } else if (strEqual(parts[0], "hello")) {
        msnprintf(buffer, sizeof(buffer), "Hello, %s!", name);
        terminal_writestring(buffer, true);
        msnprintf(buffer, sizeof(buffer), "You are running %s (%s).", SYS, VERSION);
        terminal_writestring(buffer, true);
    } else if (strEqual(parts[0], "clear")) {
        terminal_initialize();
    } else if (strEqual(parts[0], "echo")) {
        if (count < 2) {
            terminal_writestring("Error: ECHO NEEDS ARGUMENTS", true);
        } else {
            for (int i = 1; i < count; i++) {
                terminal_writestring(parts[i], false);
                if (i < count - 1) {
                    terminal_writestring(" ", false);
                }
            }
            terminal_newline();
        }
    } else {
        terminal_writestring("Error: UNKNOWN COMMAND", true);
    }
    //terminal_writestring(command, true);
    // int_to_str(terminal_row, buffer);
    // terminal_writestring(buffer, true);
}