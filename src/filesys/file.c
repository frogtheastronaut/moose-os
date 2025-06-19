//#include <stdio.h>
#include "../kernel/include/tty.h"
#include "../lib/lib.h"

#define MAX_NAME_LEN 16
#define MAX_CONTENT 64
#define MAX_CHILDREN 8
#define MAX_NODES 64

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

// mkdir
int filesys_mkdir(const char* name) {
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1;
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
    if (cwd->folder.childCount >= MAX_CHILDREN) return -1;
    FileSystemNode* node = allocNode();
    if (!node) return -1;
    copyStr(node->name, name);
    node->type = FILE_NODE;
    node->parent = cwd;
    copyStr(node->file.content, content);
    cwd->folder.children[cwd->folder.childCount++] = node;
    return 0;
}

// LS
void filesys_ls() {
    //printf("Contents of %s:\n", cwd->name);
    msnprintf(buffer, sizeof(buffer), "Contents of %s:", cwd->name);
    terminal_writestring(buffer, true);
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
    return -1; // Not found
}

// Open file
void filesys_cat(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            //printf("File %s content: %s\n", name, child->file.content);
            msnprintf(buffer, sizeof(buffer), "%s", child->file.content);
            terminal_writestring(buffer, true);
            return;
        }
    }
    //printf("File not found.\n");
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


// Demo
void demo() {
    filesys_mkdir("docs");
    filesys_mkdir("src");
    filesys_mkfile("hello.txt", "Hello, world!");
    filesys_ls();

    //printf("\nChanging into 'docs'\n");
    terminal_writestring("Changing into 'docs'", true);
    filesys_cd("docs");
    filesys_mkfile("info.txt", "Docs folder file.");
    filesys_ls();

    //printf("\nGoing back to root\n");
    terminal_writestring("Going back to root", true);
    filesys_cd("..");
    filesys_ls();
    terminal_writestring("Removing files", true);
    filesys_rmdir("src");
    filesys_rm("hello.txt");
    filesys_ls();

    //printf("\nOpening hello.txt:\n");
    terminal_writestring("Opening hello.txt:", true);
    filesys_cat("hello.txt");

    return;
}
