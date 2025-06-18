//#include <stdio.h>
#include "../kernel/include/tty.h"

#define MAX_NAME_LEN 16
#define MAX_CONTENT 64
#define MAX_CHILDREN 8
#define MAX_NODES 64

typedef char *va_list;
#define _INTSIZEOF(n)    ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap,v)   ( ap = (va_list)&v + _INTSIZEOF(v) )
#define va_arg(ap,t)     ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)       ( ap = (va_list)0 )

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

// Basic string copy
void copyStr(char* dest, const char* src) {
    int i = 0;
    while (src[i] && i < MAX_NAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Basic string compare
int strEqual(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return 0;
        i++;
    }
    return a[i] == b[i];
}
int msnprintf(char *buffer, int size, const char *format, ...) {
    va_list args;
    va_start(args, format);

    int written = 0;
    char *buf_ptr = buffer;
    const char *fmt_ptr = format;

    while (*fmt_ptr) {
        if (*fmt_ptr == '%' && *(fmt_ptr + 1) == 's') {
            const char *str = va_arg(args, const char *);
            while (*str) {
                if (written + 1 < size) {
                    *buf_ptr++ = *str;
                }
                written++;
                str++;
            }
            fmt_ptr += 2;
        } else if (*fmt_ptr == '%' && *(fmt_ptr + 1) == '%') {
            if (written + 1 < size) {
                *buf_ptr++ = '%';
            }
            written++;
            fmt_ptr += 2;
        } else {
            if (written + 1 < size) {
                *buf_ptr++ = *fmt_ptr;
            }
            written++;
            fmt_ptr++;
        }
    }
    if (size > 0) {
        *buf_ptr = '\0';
    }
    va_end(args);
    return written;
}

// Allocate node
FileSystemNode* allocNode() {
    if (nodeCount >= MAX_NODES) return NULL;
    return &nodePool[nodeCount++];
}

// Initialize filesystem
void initFileSystem() {
    root = allocNode();
    if (!root) return;
    copyStr(root->name, "/");
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;
    cwd = root;
}

// Create folder
int createFolder(const char* name) {
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
int createFile(const char* name, const char* content) {
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

// List directory
void listDirectory() {
    //printf("Contents of %s:\n", cwd->name);
    msnprintf(buffer, sizeof(buffer), "Contents of %s:", cwd->name);
    terminal_writestring(buffer, true);
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        msnprintf(buffer, sizeof(buffer), " [%s] %s", child->type == FOLDER_NODE ? "DIR" : "FILE", child->name);
        terminal_writestring(buffer, true);
    }
}

// Change directory
int changeDirectory(const char* name) {
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

// Show file content
void showFile(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        FileSystemNode* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            //printf("File %s content: %s\n", name, child->file.content);
            msnprintf(buffer, sizeof(buffer), "File %s content: %s", name, child->file.content);
            terminal_writestring(buffer, true);
            return;
        }
    }
    //printf("File not found.\n");
    terminal_writestring("Error: File not found.", true);
}

// Demo
void demo() {
    initFileSystem();
    createFolder("docs");
    createFolder("src");
    createFile("hello.txt", "Hello, world!");
    listDirectory();

    //printf("\nChanging into 'docs'\n");
    terminal_writestring("Changing into 'docs'", true);
    changeDirectory("docs");
    createFile("info.txt", "Docs folder file.");
    listDirectory();

    //printf("\nGoing back to root\n");
    terminal_writestring("Going back to root", true);
    changeDirectory("..");
    listDirectory();

    //printf("\nOpening hello.txt:\n");
    terminal_writestring("Opening hello.txt:", true);
    showFile("hello.txt");

    return;
}
