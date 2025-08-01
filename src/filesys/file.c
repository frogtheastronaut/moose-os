/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.

    Deprecated content are commented, but not removed for nostalg- I mean, safety purposes.
    Yes this is a REALLY old file. Be aware some comment styles don't really match up.
*/

#include "../kernel/include/tty.h"
#include "../lib/lib.h"

// defs
#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

// file/folder
typedef enum {
    FILE_NODE,
    FOLDER_NODE
} NodeType;

// this is a file 
typedef struct File {
    char name[MAX_NAME_LEN];
    NodeType type;
    struct File* parent;
    union {
        struct {
            char content[MAX_CONTENT];
        } file;
        struct {
            struct File* children[MAX_CHILDREN];
            int childCount;
        } folder;
    };
} File;

// system
File filesys[MAX_NODES];
int fileCount = 0;

char buffer[256];

// root and cwd
File* root;
File* cwd;


// alloc file
File* allocFile() {
    if (fileCount >= MAX_NODES) return NULL;
    return &filesys[fileCount++];
}

// innit
void filesys_init() {
    root = allocFile();
    if (!root) return;
    copyStr(root->name, "/");
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;
    cwd = root;
}

// check if name in cwd
bool nameInCWD(const char* name, NodeType type) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == type && strEqual(child->name, name)) {
            return true;
        }
    }
    return false;
}

// mkdir
int filesys_mkdir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // invalid name
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1; // too many
    if (nameInCWD(name, FOLDER_NODE)) return -3; // duplicate

    File* node = allocFile();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FOLDER_NODE;
    node->parent = cwd;
    node->folder.childCount = 0;
    cwd->folder.children[cwd->folder.childCount++] = node;
    return 0;
}

// Create file
int filesys_mkfile(const char* name, const char* content) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // invalid name - too long or empty
    if (!content || strlen(content) >= MAX_CONTENT) return -3; // content too BIG
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1; // too many
    if (nameInCWD(name, FILE_NODE)) return -4; // duplicate file name

    File* node = allocFile();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FILE_NODE;
    node->parent = cwd;
    // copyStr from lib.c to copy content over
    int i = 0;
    while (content[i] && i < MAX_CONTENT - 1) {
        node->file.content[i] = content[i];
        i++;
    }
    node->file.content[i] = '\0';
    cwd->folder.children[cwd->folder.childCount++] = node;
    return 0;
}

// // LS
// void filesys_ls() {
//     msnprintf(buffer, sizeof(buffer), "Contents of %s:", cwd->name);
//     terminal_writestring(buffer, true);
//     if (cwd->folder.childCount == 0) {
//         terminal_writestring(" (empty)", true);
//     }
//     for (int i = 0; i < cwd->folder.childCount; i++) {
//         File* child = cwd->folder.children[i];
//         msnprintf(buffer, sizeof(buffer), " [%s] %s", child->type == FOLDER_NODE ? "DIR" : "FILE", child->name);
//         terminal_writestring(buffer, true);
//     }
// }

// CD
int filesys_cd(const char* name) {
    if (strEqual(name, "..")) {
        // go back to parent
        if (cwd->parent != NULL) cwd = cwd->parent;
        return 0;
    }
    // *cd's to folder*
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            cwd = child;
            return 0;
        }
    }
    // nup not found
    //terminal_writestring("Error: Directory not found.", true);
    return -1; // Not found
}

// // Open file
// void filesys_cat(const char* name) {
//     for (int i = 0; i < cwd->folder.childCount; i++) {
//         File* child = cwd->folder.children[i];
//         if (child->type == FILE_NODE && strEqual(child->name, name)) {
//             msnprintf(buffer, sizeof(buffer), "%s", child->file.content);
//             terminal_writestring(buffer, true);
//             return;
//         }
//     }
//     terminal_writestring("Error: File not found.", true);
// }
int filesys_rm(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            //terminal_writestring("File removed successfully.", true);
            return 0;
        }
    }
    return -1; // Not found or not a file
}
int filesys_rmdir(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            if (child->folder.childCount > 0) {
                //terminal_writestring("Error: Folder is not empty", true);
                return -2; // Directory not empty
            }
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            //terminal_writestring("Directory removed successfully.", true);
            return 0;
        }
    }
    return -1; // Not found or not a directory
}
// void filesys_pwd(const bool newline) {
//     char path[256] = "";
//     char temp[256];
//     File* node = cwd;

//     while (node != NULL && node->parent != NULL) {
//         // Prepend "/name" to the path
//         msnprintf(temp, sizeof(temp), "/%s%s", node->name, path);
//         copyStr(path, temp);
//         node = node->parent;
//     }

//     if (path[0] == '\0') {
//         copyStr(path, "/"); // Path is root
//     }

//     terminal_writestring(path, newline);
// }
// size_t filesys_pwdlen() {
//     char path[256] = "";
//     char temp[256];
//     File* node = cwd;

//     while (node != NULL && node->parent != NULL) {
//         // Prepend "/name" to the path
//         msnprintf(temp, sizeof(temp), "/%s%s", node->name, path);
//         copyStr(path, temp);
//         node = node->parent;
//     }

//     if (path[0] == '\0') {
//         copyStr(path, "/"); // Path is root
//     }

//     return strlen(path); // Return length of path
// }

int filesys_editfile(const char* name, const char* new_content) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Replace content
            int j = 0;
            while (new_content[j] && j < MAX_CONTENT - 1) {
                child->file.content[j] = new_content[j];
                j++;
            }
            child->file.content[j] = '\0';
            //terminal_writestring("File content updated.", true);
            return 0;
        }
    }
    //terminal_writestring("Error: File not found.", true);
    return -1;
}