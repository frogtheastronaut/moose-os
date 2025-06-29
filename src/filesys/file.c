/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.
*/

#include "../kernel/include/tty.h"
#include "../lib/lib.h"

#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

typedef enum {
    FILE_NODE,
    FOLDER_NODE
} NodeType;

typedef struct FileSystemNode {
    char name[MAX_NAME_LEN];
    NodeType type;
    struct FileSystemNode* parent;
    union {
        struct {
            char content[MAX_CONTENT];
        } file;
        struct {
            struct FileSystemNode* children[MAX_CHILDREN];
            int childCount;
        } folder;
    };
} FileSystemNode;

// Node pool (static memory)
FileSystemNode nodePool[MAX_NODES];
int nodeCount = 0;

char buffer[256];

// Current working directory
FileSystemNode* root;
FileSystemNode* cwd;


// Allocate node
FileSystemNode* allocNode() {
    if (nodeCount >= MAX_NODES) return NULL;
    return &nodePool[nodeCount++];
}

// Initialise
void filesys_init() {
    root = allocNode();
    if (!root) return;
    copyStr(root->name, "/");
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;
    cwd = root;
}

// Helper: Check for duplicate name in current directory
bool nameExistsInCwd(const char* name, NodeType type) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == type && strEqual(child->name, name)) {
            return true;
        }
    }
    return false;
}

// mkdir
int filesys_mkdir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2;
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1;
    if (nameExistsInCwd(name, FOLDER_NODE)) return -3; // Duplicate folder name

    FileSystemNode* node = allocNode();
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
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2;
    if (!content || strlen(content) >= MAX_CONTENT) return -3;
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1;
    if (nameExistsInCwd(name, FILE_NODE)) return -4; // Duplicate file name

    FileSystemNode* node = allocNode();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FILE_NODE;
    node->parent = cwd;
    // Use copyStr for content, but you may want to make a version for MAX_CONTENT
    int i = 0;
    while (content[i] && i < MAX_CONTENT - 1) {
        node->file.content[i] = content[i];
        i++;
    }
    node->file.content[i] = '\0';
    cwd->folder.children[cwd->folder.childCount++] = node;
    return 0;
}

// LS
void filesys_ls() {
    msnprintf(buffer, sizeof(buffer), "Contents of %s:", cwd->name);
    terminal_writestring(buffer, true);
    if (cwd->folder.childCount == 0) {
        terminal_writestring(" (empty)", true);
    }
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        msnprintf(buffer, sizeof(buffer), " [%s] %s", child->type == FOLDER_NODE ? "DIR" : "FILE", child->name);
        terminal_writestring(buffer, true);
    }
}

// CD
int filesys_cd(const char* name) {
    if (strEqual(name, "..")) {
        if (cwd->parent != NULL) cwd = cwd->parent;
        return 0;
    }

    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            cwd = child;
            return 0;
        }
    }
    terminal_writestring("Error: Directory not found.", true);
    return -1; // Not found
}

// Open file
void filesys_cat(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            msnprintf(buffer, sizeof(buffer), "%s", child->file.content);
            terminal_writestring(buffer, true);
            return;
        }
    }
    terminal_writestring("Error: File not found.", true);
}
int filesys_rm(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            terminal_writestring("File removed successfully.", true);
            return 0;
        }
    }
    return -1; // Not found or not a file
}
int filesys_rmdir(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            if (child->folder.childCount > 0) {
                terminal_writestring("Error: Folder is not empty", true);
                return -2; // Directory not empty
            }
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            terminal_writestring("Directory removed successfully.", true);
            return 0;
        }
    }
    return -1; // Not found or not a directory
}
void filesys_pwd(const bool newline) {
    char path[256] = "";
    char temp[256];
    FileSystemNode* node = cwd;

    while (node != NULL && node->parent != NULL) {
        // Prepend "/name" to the path
        msnprintf(temp, sizeof(temp), "/%s%s", node->name, path);
        copyStr(path, temp);
        node = node->parent;
    }

    if (path[0] == '\0') {
        copyStr(path, "/"); // Path is root
    }

    terminal_writestring(path, newline);
}
size_t filesys_pwdlen() {
    char path[256] = "";
    char temp[256];
    FileSystemNode* node = cwd;

    while (node != NULL && node->parent != NULL) {
        // Prepend "/name" to the path
        msnprintf(temp, sizeof(temp), "/%s%s", node->name, path);
        copyStr(path, temp);
        node = node->parent;
    }

    if (path[0] == '\0') {
        copyStr(path, "/"); // Path is root
    }

    return strlen(path); // Return length of path
}

int filesys_editfile(const char* name, const char* new_content) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Replace content
            int j = 0;
            while (new_content[j] && j < MAX_CONTENT - 1) {
                child->file.content[j] = new_content[j];
                j++;
            }
            child->file.content[j] = '\0';
            terminal_writestring("File content updated.", true);
            return 0;
        }
    }
    terminal_writestring("Error: File not found.", true);
    return -1;
}