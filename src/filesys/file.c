/*
    Moose Operating System
    Copyright 2025 Ethan Zhang, All rights reserved.

    @todo add disk I/O, dynamic file handling.
*/

// Includes
#include "../kernel/include/tty.h"
#include "../lib/lib.h"

// Definitions
#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

// Nodes can either be Files or Folders
typedef enum {
    FILE_NODE,
    FOLDER_NODE
} NodeType;

// File contains:
// Name ( @todo Remove limitations)
// Its type (file/folder)
// Its parent folder
// Its content ( @todo Remove limitations)
// Children, and childcount ( @todo Same as above)
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

// This is the filesystem ( @todo Remove limitations)
File filesys[MAX_NODES];
int fileCount = 0;

// Buffer used for multiple purposes
char buffer[256];

// Root and Current Working Director
File* root;
File* cwd;


// It's like Malloc, but for files.
// Allocates a new File structure from a filesys
// @return pointer to File, or NULL if full
File* allocFile() {
    if (fileCount >= MAX_NODES) return NULL;
    return &filesys[fileCount++];
}

// Initialise filesystem.
void filesys_init() {
    root = allocFile();
    if (!root) return; // Root does not exist
    copyStr(root->name, "/"); // Root is /

    // Initialise root
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;

    // We start at root
    cwd = root;
}

// check if file name in CWD
// @return true if exists, false if not
bool nameInCWD(const char* name, NodeType type) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        // Check if file type and name match up
        if (child->type == type && strEqual(child->name, name)) {
            // Exists
            return true;
        }
    }
    // Does not exist
    return false;
}

// Make directory
// @return number depending on success
int filesys_mkdir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1; // Too many files/folders
    if (nameInCWD(name, FOLDER_NODE)) return -3; // Duplicate file/folder

    // This is really similar to how we made a root folder.
    File* node = allocFile();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FOLDER_NODE;
    node->parent = cwd;
    node->folder.childCount = 0;
    cwd->folder.children[cwd->folder.childCount++] = node;

    // Success
    return 0;
}

// Create file
// @return number depending on success
int filesys_mkfile(const char* name, const char* content) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name - too long/empty
    if (!content || strlen(content) >= MAX_CONTENT) return -3; // Content too large @todo remove limitations and add dynamic file sizes.
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1; // Too many files
    if (nameInCWD(name, FILE_NODE)) return -4; // Duplicate file name

    // Same as mkdir
    File* node = allocFile();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FILE_NODE;
    node->parent = cwd;
    int i = 0;
    while (content[i] && i < MAX_CONTENT - 1) {
        node->file.content[i] = content[i];
        i++;
    }
    node->file.content[i] = '\0';
    cwd->folder.children[cwd->folder.childCount++] = node;

    // Success
    return 0;
}

// Change directory
// @return 0 on success, -1 on failure
int filesys_cd(const char* name) {
    // .. means to go back to parent folder
    if (strEqual(name, "..")) {
        if (cwd->parent != NULL) cwd = cwd->parent;
        return 0;
    }
    // CD to folder
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            cwd = child;
            return 0;
        }
    }
    return -1; // Not found
}

// Remove file
// @return 0 on success, -1 on failure
int filesys_rm(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            return 0;
        }
    }
    return -1; // Not found or not a file (is folder)
}

// Remove folder
// @return 0 on success, -1 on failure, -2 if not empty
int filesys_rmdir(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FOLDER_NODE && strEqual(child->name, name)) {
            if (child->folder.childCount > 0) {
                return -2; // Directory not empty
            }
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            return 0;
        }
    }
    return -1; // Not found/not a directory (is a file)
}
// Edit file content
// @return 0 on success, -1 on failure
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
            return 0;
        }
    }
    return -1;
}