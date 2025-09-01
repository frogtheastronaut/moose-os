#ifndef FILE_H
#define FILE_H

#include <stdint.h>
// #include "../kernel/include/tty.h"
#include "../lib/lib.h"
#include "../kernel/include/disk.h"

// Definitions
#define MAX_NAME_LEN 128
#define MAX_CONTENT 4096
#define MAX_CHILDREN 4096
#define MAX_NODES 4096

// Disk filesystem constants
#define FILESYSTEM_SIGNATURE 0x4D4F4F53  // "MOOS" in little endian
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

// On-disk inode structure (optimized to fit in 256 bytes)
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
    char name[64];              // File/directory name (reduced from 128)
    uint8_t reserved[32];       // Reserved space to make it exactly 256 bytes
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
/**
 * @todo Remove limitations
 */
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

// This is the filesystem - now using dynamic allocation
/** Dynamic file allocation implemented */
// extern File filesys[MAX_NODES];  // Removed - now using dynamic allocation
extern int fileCount;

// Buffer used for multiple purposes
extern char buffer[256];

// Root and Current Working Director
extern File* root;
extern File* cwd;

// Disk filesystem globals
extern superblock_t *superblock;
extern uint8_t filesystem_mounted;
extern uint8_t boot_drive;

// Functions
File* allocFile(void);               // Dynamic file allocator
void freeFile(File* file);           // Free a file and return to pool
int filesys_mkdir(const char* name);
int filesys_mkfile(const char* name, const char* content);
int filesys_cd(const char* name);
int filesys_rm(const char* name);
int filesys_rmdir(const char* name);
int filesys_editfile(const char* name, const char* new_content);
void filesys_init(void);

// Disk filesystem functions
int filesys_mount(uint8_t drive);
int filesys_sync(void);
int filesys_format(uint8_t drive);
int filesys_save_to_disk(void);
int filesys_load_from_disk(void);
uint32_t allocate_inode(void);
uint32_t allocate_data_block(void);
void free_inode(uint32_t inode_num);
void free_data_block(uint32_t block_num);
int write_inode_to_disk(uint32_t inode_num, disk_inode_t *inode);
int read_inode_from_disk(uint32_t inode_num, disk_inode_t *inode);
File* convert_disk_inode_to_memory(disk_inode_t *disk_inode);

// Disk utility functions
int filesys_get_disk_info(char *info_buffer, int buffer_size);
int filesys_get_memory_stats(char *stats_buffer, int buffer_size);
int filesys_disk_status(void);
void filesys_flush_cache(void);

#endif
