/*
    MooseOS File System
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/
/*
    ========================= OS THEORY =========================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.

    File systems are the operating system's method of ordering data on persistent storage devices like disks.
    
    The fundamental operations of any filesystem are:

    - Tracking the available storage space
    - Tracking which block or blocks of data belong to which files
    - Creating new files
    - Reading data from existing files into memory
    - Updating the data in the files
    - Deleting existing files
    
    MooseOS' FILE ALLOCATION:
    In MooseOS, each file/directory has an inode with file metadata. File content is stored in single disk blocks (up to 512 bytes per file currently).
    They are persistent as the inode table is saved on disk.
    
    INODES:
    The OSDev wiki describes inodes as follows:
    "inodes (information nodes) are a crucial design element in most Unix file systems: 
    Each file is made of data blocks (the sectors that contains your raw data bits), 
    index blocks (containing pointers to data blocks so that you know which sector is the 
    nth in the sequence), and one inode block.

    "The inode is the root of the index blocks, and can also be the sole index block if the 
    file is small enough. Moreover, as Unix file systems support hard links (the same file may 
    appear several times in the directory tree), inodes are a natural place to store Metadata 
    such as file size, owner, creation/access/modification times, locks, etc."

    Source: https://wiki.osdev.org/File_Systems 
*/

// Includes
#include "../include/file.h"
#include "../include/file_alloc.h"

// Current file count
int fileCount = 0;

// Buffer used for multiple purposes
char buffer[256];

// Root and Current Working Directory
File* root;
File* cwd;

// Superblock
superblock_t *superblock = NULL;

// 1 if filesystem is mounted, 0 if not
// Currently, it's not mounted
uint8_t filesystem_mounted = 0;

// Currently, boot drive does not exist
uint8_t boot_drive = 0;

// Disk buffer of sector size
uint8_t disk_buffer[SECTOR_SIZE];

// Cache
superblock_t sb_cache;

/**
 * Set file content
 */
int set_file_content(File* file, const char* content) {
    if (!file || file->type != FILE_NODE || !content) return -1;
    
    size_t new_size = strlen(content);
    
    // Free old content
    if (file->file.content) {
        free(file->file.content);
    }
    
    // Allocate new content (with null terminator)
    file->file.content = (char*)malloc(new_size + 1);
    if (!file->file.content) {
        file->file.content_size = 0;
        file->file.content_capacity = 0;
        return -1; // Out of memory
    }
    
    // Copy content
    for (size_t i = 0; i < new_size; i++) {
        file->file.content[i] = content[i];
    }
    file->file.content[new_size] = '\0';
    
    file->file.content_size = new_size;
    file->file.content_capacity = new_size + 1;
    
    return 0;
}

/**
 * Add child to directory
 * 
 * @returns 0 on success, -1 on failure.
 */
int add_child_to_dir(File* dir, File* child) {
    if (!dir || !child || dir->type != FOLDER_NODE) return -1;
    
    // Initialize children array if first child
    if (!dir->folder.children) {
        /**
         * @note We could make this bigger
         */
        dir->folder.capacity = 4; 
        dir->folder.children = (File**)malloc(dir->folder.capacity * sizeof(File*));
        if (!dir->folder.children) return -1;
        dir->folder.childCount = 0;
    }
    
    // Grow array if needed
    if (dir->folder.childCount >= dir->folder.capacity) {
        int new_capacity = dir->folder.capacity * 2;
        File** new_children = (File**)realloc(dir->folder.children, new_capacity * sizeof(File*));
        if (!new_children) return -1; // Out of memory
        
        dir->folder.children = new_children;
        dir->folder.capacity = new_capacity;
    }
    
    // Add child
    dir->folder.children[dir->folder.childCount] = child;
    dir->folder.childCount++;
    child->parent = dir;
    
    return 0;
}

/**
 * Remove child from directory
 * @return 0 on success, -1 on failure
 */
int remove_child_from_dir(File* dir, File* child) {
    if (!dir || !child || dir->type != FOLDER_NODE || !dir->folder.children) return -1;
    
    // Find child index
    int child_index = -1;
    for (int i = 0; i < dir->folder.childCount; i++) {
        if (dir->folder.children[i] == child) {
            child_index = i;
            break;
        }
    }
    
    if (child_index == -1) return -1; // Child not found
    
    // Shift remaining children left
    for (int i = child_index; i < dir->folder.childCount - 1; i++) {
        dir->folder.children[i] = dir->folder.children[i + 1];
    }
    dir->folder.childCount--;
    
    return 0;
}

 /**
 * Check if file name in CWD
 * @return true if exists, false if not
 */
bool name_in_CWD(const char* name, NodeType type) {
    if (!cwd || cwd->type != FOLDER_NODE || !cwd->folder.children) return false;
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == type && strEqual(child->name, name)) {
            return true;
        }
    }
    return false;
}

/**
 * Write inode to disk with bounds checking
 */
int write_inode_to_disk(uint32_t inode_num, disk_inode_t *inode) {
    if (inode_num == 0 || inode_num >= MAX_DISK_INODES) return -1;
    if (!inode) return -1;
    
    uint32_t sector = INODE_TABLE_SECTOR + (inode_num / INODES_PER_SECTOR);
    uint32_t inode_offset_in_sector = inode_num % INODES_PER_SECTOR;
    uint32_t byte_offset = inode_offset_in_sector * sizeof(disk_inode_t);
    
    // Validate that inode fits in sector
    if (byte_offset + sizeof(disk_inode_t) > SECTOR_SIZE) {
        return -7; // Inode doesn't fit in sector
    }
    
    // Read sector, modify, write back
    if (disk_read_sector(boot_drive, sector, disk_buffer) != 0) {
        return -1;
    }
    
    // Copy inode data safely
    uint8_t *src = (uint8_t*)inode;
    uint8_t *dst = disk_buffer + byte_offset;
    for (uint32_t i = 0; i < sizeof(disk_inode_t); i++) {
        dst[i] = src[i];
    }
    
    if (disk_write_sector(boot_drive, sector, disk_buffer) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * Read inode from disk with bounds checking
 */
int read_inode_from_disk(uint32_t inode_num, disk_inode_t *inode) {
    if (inode_num == 0 || inode_num >= MAX_DISK_INODES) return -1;
    if (!inode) return -1;
    
    uint32_t sector = INODE_TABLE_SECTOR + (inode_num / INODES_PER_SECTOR);
    uint32_t inode_offset_in_sector = inode_num % INODES_PER_SECTOR;
    uint32_t byte_offset = inode_offset_in_sector * sizeof(disk_inode_t);
    
    // Validate that inode fits in sector
    if (byte_offset + sizeof(disk_inode_t) > SECTOR_SIZE) {
        return -7; // Inode doesn't fit in sector
    }
    
    if (disk_read_sector(boot_drive, sector, disk_buffer) != 0) {
        return -1;
    }
    
    // Copy inode data safely
    uint8_t *src = disk_buffer + byte_offset;
    uint8_t *dst = (uint8_t*)inode;
    for (uint32_t i = 0; i < sizeof(disk_inode_t); i++) {
        dst[i] = src[i];
    }
    
    return 0;
}

/**
 * Convert a memory File structure to disk inode format
 */
static int memory_to_inode(File *memory_file, disk_inode_t *disk_inode, uint32_t inode_num, uint32_t parent_inode) {
    if (!memory_file || !disk_inode) return -1;
    
    // Clear the disk inode
    for (uint32_t i = 0; i < sizeof(disk_inode_t); i++) {
        ((uint8_t*)disk_inode)[i] = 0;
    }
    
    disk_inode->signature = FILESYSTEM_SIGNATURE;
    disk_inode->inode_number = inode_num;
    disk_inode->type = memory_file->type;
    disk_inode->parent_inode = parent_inode;
    
    // Copy name
    int name_len = 0;
    while (name_len < MAX_NAME_LEN - 1 && memory_file->name[name_len] != '\0') {
        disk_inode->name[name_len] = memory_file->name[name_len];
        name_len++;
    }
    disk_inode->name[name_len] = '\0';
    
    if (memory_file->type == FILE_NODE) {
        // Handle file content
        if (memory_file->file.content) {
            disk_inode->size = memory_file->file.content_size;
        } else {
            disk_inode->size = 0;
        }
        
        // Write content to data blocks (if file has content)
        if (disk_inode->size > 0 && memory_file->file.content) {
            uint32_t data_block = allocate_data_block();
            if (data_block > 0) {
                disk_inode->data_blocks[0] = data_block;
                
                // Write file content to disk block
                uint8_t content_buffer[SECTOR_SIZE];
                for (int i = 0; i < SECTOR_SIZE; i++) {
                    content_buffer[i] = 0; // Clear buffer
                }
                
                // Copy content (limited to sector size)
                uint32_t content_len = disk_inode->size;
                if (content_len > SECTOR_SIZE - 1) {
                    content_len = SECTOR_SIZE - 1; // Leave space for null terminator
                }
                
                for (uint32_t i = 0; i < content_len; i++) {
                    content_buffer[i] = memory_file->file.content[i];
                }
                
                // Write content to disk
                if (disk_write_sector(boot_drive, data_block, content_buffer) != 0) {
                    // Failed to write content, free the block
                    free_data_block(data_block);
                    disk_inode->data_blocks[0] = 0;
                }
            }
        }
    } else {
        // Handle directory
        disk_inode->size = memory_file->folder.childCount;
        disk_inode->child_count = 0; // Will be filled by recursive save
    }
    
    return 0;
}

/**
 * Recursively save directory tree to disk
 * 
 */
int save_directory_recursive(File *dir, uint32_t dir_inode_num, uint32_t parent_inode) {
    if (!dir || dir->type != FOLDER_NODE) return -1;
    
    disk_inode_t dir_disk_inode;
    if (memory_to_inode(dir, &dir_disk_inode, dir_inode_num, parent_inode) != 0) {
        return -1;
    }
    
    // Save directory inode
    if (write_inode_to_disk(dir_inode_num, &dir_disk_inode) != 0) {
        return -1;
    }
    
    // Save children
    for (int i = 0; i < dir->folder.childCount && i < MAX_CHILDREN_PER_DIR; i++) {
        if (!dir->folder.children || !dir->folder.children[i]) continue;
        
        File *child = dir->folder.children[i];
        uint32_t child_inode = allocate_inode();
        if (child_inode == 0) continue;
        
        if (child->type == FILE_NODE) {
            disk_inode_t child_disk_inode;
            if (memory_to_inode(child, &child_disk_inode, child_inode, dir_inode_num) == 0) {
                write_inode_to_disk(child_inode, &child_disk_inode);
                dir_disk_inode.child_inodes[dir_disk_inode.child_count++] = child_inode;
            }
        } else {
            // Recursively save subdirectory
            if (save_directory_recursive(child, child_inode, dir_inode_num) == 0) {
                dir_disk_inode.child_inodes[dir_disk_inode.child_count++] = child_inode;
            }
        }
    }
    
    // Update directory with child references
    write_inode_to_disk(dir_inode_num, &dir_disk_inode);
    
    return 0;
}

/**
 * Convert disk inode to memory File structure
 */
static File* convert_disk_to_memory_file(disk_inode_t *disk_inode) {
    if (!disk_inode || disk_inode->signature != FILESYSTEM_SIGNATURE) return NULL;
    
    File *memory_file = file_alloc();
    if (!memory_file) return NULL;
    
    // Copy basic information
    copyStr(memory_file->name, disk_inode->name);
    memory_file->type = disk_inode->type;
    
    if (disk_inode->type == FILE_NODE) {
        // Load file content from disk blocks
        if (disk_inode->size > 0 && disk_inode->data_blocks[0] > 0) {
            uint8_t content_buffer[SECTOR_SIZE];
            
            if (disk_read_sector(boot_drive, disk_inode->data_blocks[0], content_buffer) == 0) {
                // Allocate content
                memory_file->file.content = (char*)malloc(disk_inode->size + 1);
                if (memory_file->file.content) {
                    // Copy content to memory file
                    uint32_t content_len = disk_inode->size;
                    if (content_len > SECTOR_SIZE - 1) {
                        content_len = SECTOR_SIZE - 1; // Prevent buffer overflow
                    }
                    
                    for (uint32_t i = 0; i < content_len; i++) {
                        memory_file->file.content[i] = content_buffer[i];
                    }
                    memory_file->file.content[content_len] = '\0';
                    memory_file->file.content_size = content_len;
                    memory_file->file.content_capacity = disk_inode->size + 1;
                } else {
                    // Failed to allocate content memory
                    memory_file->file.content = NULL;
                    memory_file->file.content_size = 0;
                    memory_file->file.content_capacity = 0;
                }
            } else {
                // Failed to read content
                memory_file->file.content = NULL;
                memory_file->file.content_size = 0;
                memory_file->file.content_capacity = 0;
            }
        } else {
            // Empty file
            memory_file->file.content = NULL;
            memory_file->file.content_size = 0;
            memory_file->file.content_capacity = 0;
        }
    } else {
        // For directories - initialize empty
        memory_file->folder.children = NULL;
        memory_file->folder.childCount = 0;
        memory_file->folder.capacity = 0;
    }
    
    return memory_file;
}

/**
 * Recursively load directory tree from disk
 */
int load_directory_recursive(uint32_t inode_num, File *parent) {
    disk_inode_t disk_inode;
    if (read_inode_from_disk(inode_num, &disk_inode) != 0) {
        return -1;
    }
    
    if (disk_inode.signature != FILESYSTEM_SIGNATURE) {
        return -1;
    }
    
    File *memory_file = convert_disk_to_memory_file(&disk_inode);
    if (!memory_file) return -1;
    
    memory_file->parent = parent;
    
    // Add to parent directory if parent exists
    if (parent && parent->type == FOLDER_NODE) {
        if (add_child_to_dir(parent, memory_file) != 0) {
            file_free(memory_file);
            return -1;
        }
    }
    
    // If this is a directory, load its children
    if (disk_inode.type == FOLDER_NODE) {
        for (uint32_t i = 0; i < disk_inode.child_count && i < MAX_CHILDREN_PER_DIR; i++) {
            uint32_t child_inode = disk_inode.child_inodes[i];
            if (child_inode > 0) {
                load_directory_recursive(child_inode, memory_file);
            }
        }
    }
    
    return 0;
}
