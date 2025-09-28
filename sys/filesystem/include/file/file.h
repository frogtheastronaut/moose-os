/*
    MooseOS File 
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include "libc/lib.h"
#include "ata/ata.h"
#include "heap/heap.h"

// definitions
#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

// disk filesystem constants
#define FILESYSTEM_SIGNATURE 0x4D4F4F53  // spells "MOOS"
#define SUPERBLOCK_SECTOR 0
#define INODE_TABLE_SECTOR 1
#define DATA_BLOCKS_START_SECTOR 100
#define SECTORS_PER_INODE 1
#define DISK_INODE_SIZE 256         // size of disk_inode structure
#define DIR_ENTRY_SIZE 64           // size of dir_entry structure
#define INODES_PER_SECTOR 2         // 512 / 256 = 2
#define MAX_DISK_INODES 512
#define DIR_ENTRIES_PER_SECTOR 8    // 512 / 64 = 8
#define MAX_CHILDREN_PER_DIR 16     // maximum children per directory (matches child_inodes array size)

// nodes can either be Files or Folders
/**
 * @todo: this is racism. we are assuming folders
 * are files and they get along together
 * 
 * maybe change the name of the enum
 */
typedef enum {
    FILE_NODE,
    FOLDER_NODE
} file_node;

// on-disk inode structure
typedef struct {
    uint32_t signature;         // magic number for validation
    uint32_t inode_number;      // unique inode number
    file_node type;              // file or directory
    uint32_t size;              // size in bytes (for files)
    uint32_t parent_inode;      // parent directory inode number (0 for root)
    uint32_t data_blocks[12];   // direct block pointers
    uint32_t indirect_block;    // indirect block pointer
    uint32_t created_time;      // creation timestamp
    uint32_t modified_time;     // last modification timestamp
    uint32_t child_count;       // number of children (for directories)
    uint32_t child_inodes[16];  // child inode numbers (for directories)
    char name[64];              // file/directory name
    uint8_t reserved[32];       // reserved space
} disk_inode;

// directory entry structure
/**
 * @note unused
 */
typedef struct {
    uint32_t inode_number;      // inode number of this entry
    char name[60];              // name of file/directory
} dir_entry;

// superblock structure
typedef struct {
    uint32_t signature;         // filesystem signature
    uint32_t total_sectors;     // total sectors in filesystem
    uint32_t inode_count;       // total number of inodes
    uint32_t free_inode_count;  // number of free inodes
    uint32_t data_block_count;  // total data blocks
    uint32_t free_block_count;  // number of free data blocks
    uint32_t root_inode;        // root directory inode number
    uint8_t inode_bitmap[64];   // bitmap for inode allocation (512 inodes max)
    uint8_t block_bitmap[64];   // bitmap for block allocation
} file_superblock;

// file contains:
// name 
// its type (file/folder)
// its parent folder
// its content
// children, and childcount
/**
 * @todo: why is this uppercase?
 */
typedef struct File {
    char name[MAX_NAME_LEN]; // name of file
    file_node type; // type (File or Folder)
    struct File* parent; // parent file pointer
    union {
        struct {
            char* content;
            size_t content_size;    // actual content size
            size_t content_capacity; // allocated capacity
        } file;
        struct {
            struct File** children; // array of pointers
            int childCount;
            int capacity;           // array capacity
        } folder;
    };
} File;

// external variables
extern int file_count;
extern char buffer[256];
extern File* root;
extern File* cwd;

// external disk filesystem variables
extern file_superblock *superblock;
extern uint8_t filesystem_mounted;
extern uint8_t boot_drive;

// function prototypes
File* file_alloc(void);
void file_free(File* file);
int filesystem_make_dir(const char* name);
int filesystem_make_file(const char* name, const char* content);
int fs_change_dir(const char* name);
int fs_remove(const char* name);
int fs_remove_dir(const char* name);
int fs_edit_file(const char* name, const char* new_content);
void filesystem_init(void);

// internal helper functions (used by filesystem.c)
bool name_in_CWD(const char* name, file_node type);
int set_file_content(File* file, const char* content);
int add_child_to_dir(File* dir, File* child);
int remove_child_from_dir(File* dir, File* child);
int save_directory_recursive(File *dir, uint32_t dir_inode_num, uint32_t parent_inode);
int load_directory_recursive(uint32_t inode_num, File* parent);

// external variables
extern uint8_t disk_buffer[512];  // SECTOR_SIZE = 512
extern file_superblock sb_cache;

// disk filesystem function prototypes
int filesystem_mount(uint8_t drive);
int filesystem_sync(void);
int filesystem_format(uint8_t drive);
int filesystem_save_to_disk(void);
int filesystem_load_from_disk(void);
uint32_t allocate_inode(void);
uint32_t allocate_data_block(void);
void free_inode(uint32_t inode_num);
void free_data_block(uint32_t block_num);
int write_inode_to_disk(uint32_t inode_num, disk_inode *inode);
int read_inode_from_disk(uint32_t inode_num, disk_inode *inode);

// disk utility functions
int filesystem_get_disk_info(char *info_buffer, int buffer_size);
int filesystem_get_memory_stats(char *stats_buffer, int buffer_size);
int filesystem_disk_status(void);
void filesystem_flush_cache(void);

#endif // FILE_H
