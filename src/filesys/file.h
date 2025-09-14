/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang and Contributors.

    Header file for ./file.c
*/

#ifndef FILE_H
#define FILE_H

#include <stdint.h>
// tty.h is deprecated.
// #include "../kernel/include/tty.h"
#include "../lib/lib.h"
#include "../kernel/include/disk.h"
#include "../lib/malloc.h"

// Definitions
#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

// Disk filesystem constants
#define FILESYSTEM_SIGNATURE 0x4D4F4F53  // "MOOS"
#define SUPERBLOCK_SECTOR 0
#define INODE_TABLE_SECTOR 1
#define DATA_BLOCKS_START_SECTOR 100
#define SECTORS_PER_INODE 1
#define DISK_INODE_SIZE 256         // Size of disk_inode_t structure
#define DIR_ENTRY_SIZE 64           // Size of dir_entry_t structure
#define INODES_PER_SECTOR 2         // 512 / 256 = 2
#define MAX_DISK_INODES 512
#define DIR_ENTRIES_PER_SECTOR 8    // 512 / 64 = 8
#define MAX_CHILDREN_PER_DIR 16     // Maximum children per directory (matches child_inodes array size)

// Nodes can either be Files or Folders
typedef enum {
    FILE_NODE,
    FOLDER_NODE
} NodeType;

// On-disk inode structure
typedef struct {
    uint32_t signature;         // Magic number for validation
    uint32_t inode_number;      // Unique inode number
    NodeType type;              // File or directory
    uint32_t size;              // Size in bytes (for files)
    uint32_t parent_inode;      // Parent directory inode number (0 for root)
    uint32_t data_blocks[12];   // Direct block pointers
    uint32_t indirect_block;    // Indirect block pointer
    uint32_t created_time;      // Creation timestamp
    uint32_t modified_time;     // Last modification timestamp
    uint32_t child_count;       // Number of children (for directories)
    uint32_t child_inodes[16];  // Child inode numbers (for directories)
    char name[64];              // File/directory name
    uint8_t reserved[32];       // Reserved space
} disk_inode_t;

// Directory entry structure (stored in data blocks for directories)
typedef struct {
    uint32_t inode_number;      // Inode number of this entry
    char name[60];              // Name of file/directory
} dir_entry_t;

// Superblock structure
typedef struct {
    uint32_t signature;         // Filesystem signature
    uint32_t total_sectors;     // Total sectors in filesystem
    uint32_t inode_count;       // Total number of inodes
    uint32_t free_inode_count;  // Number of free inodes
    uint32_t data_block_count;  // Total data blocks
    uint32_t free_block_count;  // Number of free data blocks
    uint32_t root_inode;        // Root directory inode number
    uint8_t inode_bitmap[64];   // Bitmap for inode allocation (512 inodes max)
    uint8_t block_bitmap[64];   // Bitmap for block allocation
} superblock_t;

// File contains:
// Name 
// Its type (file/folder)
// Its parent folder
// Its content
// Children, and childcount
typedef struct File {
    char name[MAX_NAME_LEN]; // Name of file
    NodeType type; // Type (File or Folder)
    struct File* parent; // Parent file pointer
    union {
        struct {
            char* content;
            size_t content_size;    // Actual content size
            size_t content_capacity; // Allocated capacity
        } file;
        struct {
            struct File** children; // Array of pointers
            int childCount;
            int capacity;           // Array capacity
        } folder;
    };
} File;

// External variables
extern int fileCount;
extern char buffer[256];
extern File* root;
extern File* cwd;

// Disk filesystem globals
extern superblock_t *superblock;
extern uint8_t filesystem_mounted;
extern uint8_t boot_drive;

// Functions
File* file_alloc(void);
void file_free(File* file);
int fs_make_dir(const char* name);
int fs_make_file(const char* name, const char* content);
int fs_change_dir(const char* name);
int fs_remove(const char* name);
int fs_remove_dir(const char* name);
int fs_edit_file(const char* name, const char* new_content);
void fs_init(void);

// Disk filesystem functions
int fs_mount(uint8_t drive);
int filesys_sync(void);
int fs_format(uint8_t drive);
int fs_save_to_disk(void);
int fs_load_from_disk(void);
uint32_t allocate_inode(void);
uint32_t allocate_data_block(void);
void free_inode(uint32_t inode_num);
void free_data_block(uint32_t block_num);
int write_inode_to_disk(uint32_t inode_num, disk_inode_t *inode);
int read_inode_from_disk(uint32_t inode_num, disk_inode_t *inode);
File* convert_disk_inode_to_memory(disk_inode_t *disk_inode);

// Disk utility functions
int fs_get_disk_info(char *info_buffer, int buffer_size);
int filesys_get_memory_stats(char *stats_buffer, int buffer_size);
int filesys_disk_status(void);
void filesys_flush_cache(void);

#endif
