#ifndef FILE_H
#define FILE_H

// #include "../kernel/include/tty.h"
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
extern File filesys[MAX_NODES];
extern int fileCount;

// Buffer used for multiple purposes
extern char buffer[256];


// Root and Current Working Director
extern File* root;
extern File* cwd;

// Filesystem operations
int filesys_mkdir(const char* name);
int filesys_mkfile(const char* name, const char* content);

int filesys_cd(const char* name);
int filesys_rm(const char* name);
int filesys_rmdir(const char* name);
int filesys_editfile(const char* name, const char* new_content);
void filesys_init(void);

#endif
