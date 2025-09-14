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
#include "file.h"

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
static uint8_t disk_buffer[SECTOR_SIZE];

// Cache
static superblock_t sb_cache;

/**
 * Autosaves filesystem to disk after changes
 */
static void auto_save_filesystem(void) {
    if (filesystem_mounted && superblock) {
        filesys_sync();
        fs_save_to_disk();
        filesys_flush_cache();
    }
}

/**
 * Dynamic file allocator using malloc
 * Allocates a new File structure from heap
 * @return pointer to File, or NULL if out of memory
 */
File* file_alloc() {
    File* new_file = (File*)malloc(sizeof(File));
    if (new_file) {
        // Clear the allocated memory
        uint8_t* file_ptr = (uint8_t*)new_file;
        for (int i = 0; i < sizeof(File); i++) {
            file_ptr[i] = 0;
        }
        
        // Initialize dynamic fields to NULL
        new_file->name[0] = '\0';
        new_file->type = FILE_NODE; // Will be set properly later
        new_file->parent = NULL;
        
        // Initialize union fields to NULL
        new_file->file.content = NULL;
        new_file->file.content_size = 0;
        new_file->file.content_capacity = 0;
        
        fileCount++;
    }
    return new_file;
}

/**
 * Free a file using free()
 * @param file pointer to the file to free
 */
void file_free(File* file) {
    if (!file) return;
    
    if (file->type == FILE_NODE) {
        // Free content
        if (file->file.content) {
            free(file->file.content);
            file->file.content = NULL;
        }
    } else if (file->type == FOLDER_NODE) {
        // Recursively free all children
        for (int i = 0; i < file->folder.childCount; i++) {
            if (file->folder.children && file->folder.children[i]) {
                file_free(file->folder.children[i]);
            }
        }
        // Free the children array
        if (file->folder.children) {
            free(file->folder.children);
            file->folder.children = NULL;
        }
    }
    
    // Free the file and decrease fileCount because we just deleted the file.
    free(file);
    fileCount--;
}

/**
 * Set file content
 */
static int set_file_content(File* file, const char* content) {
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
static int add_child_to_dir(File* dir, File* child) {
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
static int remove_child_from_dir(File* dir, File* child) {
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

// Initialise filesystem.
void fs_init() {
    root = file_alloc();
    if (!root) return; // Root does not exist (file_alloc failed)
    copyStr(root->name, "/"); // Root is /

    // Initialise root
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.children = NULL;
    root->folder.childCount = 0;
    root->folder.capacity = 0;

    // We start at root
    cwd = root;
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
 * Make directory
 * @return number depending on success (0 is success, others are fail)
 */
int fs_make_dir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name
    if (name_in_CWD(name, FOLDER_NODE)) return -3; // Duplicate file/folder

    File* node = file_alloc();
    if (!node) return -1;
    
    copyStr(node->name, name);
    node->type = FOLDER_NODE;
    
    // Initialize empty directory (no children allocated yet)
    node->folder.children = NULL;
    node->folder.childCount = 0;
    node->folder.capacity = 0;
    
    // Add to current directory
    if (add_child_to_dir(cwd, node) != 0) {
        file_free(node);
        return -1;
    }

    // Auto-save to disk after creating directory
    auto_save_filesystem();

    return 0;
}

/** 
 * Create file
 * @return number depending on success
 */
int fs_make_file(const char* name, const char* content) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name - too long/empty
    if (!content) return -3; // Content is NULL
    if (name_in_CWD(name, FILE_NODE)) return -4; // Duplicate file name

    File* node = file_alloc();
    if (!node) return -1;
    
    copyStr(node->name, name);
    node->type = FILE_NODE;
    
    // Set content
    if (set_file_content(node, content) != 0) {
        file_free(node);
        return -1; // Failed to allocate content
    }
    
    // Add to current directory
    if (add_child_to_dir(cwd, node) != 0) {
        file_free(node);
        return -1;
    }

    // Auto-save to disk after creating file
    auto_save_filesystem();

    return 0;
}

/** 
 * Change directory
 * @return 0 on success, -1 on failure
 */
int fs_change_dir(const char* name) {
    // .. means to go back to parent folder
    if (strEqual(name, "..")) {
        if (cwd->parent != NULL) cwd = cwd->parent;
        return 0;
    }
    
    if (!cwd->folder.children) return -1; // No children
    
    // CD to folder
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FOLDER_NODE && strEqual(child->name, name)) {
            cwd = child;
            return 0;
        }
    }
    return -1; // Not found
}

/** 
 * Remove file
 * @return 0 on success, -1 on failure
 */
int fs_remove(const char* name) {
    if (!cwd->folder.children) return -1; // No children
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FILE_NODE && strEqual(child->name, name)) {
            // Remove from directory
            if (remove_child_from_dir(cwd, child) == 0) {
                // Free the file memory
                file_free(child);
                
                // Auto-save to disk after removing file
                auto_save_filesystem();
                
                return 0;
            }
        }
    }
    return -1; // Not found or not a file (is folder)
}

/** 
 * Remove folder
 * @return 0 on success, -1 on failure, -2 if not empty
 */
int fs_remove_dir(const char* name) {
    if (!cwd->folder.children) return -1; // No children
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FOLDER_NODE && strEqual(child->name, name)) {
            if (child->folder.childCount > 0) {
                return -2; // Directory not empty
            }
            
            // Remove from directory
            if (remove_child_from_dir(cwd, child) == 0) {
                // Free the directory memory
                file_free(child);
                
                // Auto-save to disk after removing directory
                auto_save_filesystem();
                
                return 0;
            }
        }
    }
    return -1; // Not found/not a directory (is a file)
}

/** Edit file content
 * @return 0 on success, -1 on failure
 */
int fs_edit_file(const char* name, const char* new_content) {
    if (!cwd->folder.children) return -1; // No children
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FILE_NODE && strEqual(child->name, name)) {
            // Set new content
            if (set_file_content(child, new_content) == 0) {
                // Auto-save to disk after editing file
                auto_save_filesystem();
                return 0;
            }
            return -1; // Failed to set content
        }
    }
    return -1; // File not found
}

/**
 * Allocate a new inode number
 */
uint32_t allocate_inode(void) {
    if (!superblock) return 0;
    
    for (uint32_t i = 1; i < MAX_DISK_INODES; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        
        if (!(superblock->inode_bitmap[byte_index] & (1 << bit_index))) {
            // Mark as allocated
            superblock->inode_bitmap[byte_index] |= (1 << bit_index);
            superblock->free_inode_count--;
            return i;
        }
    }
    return 0; // No free inodes
}

/**
 * Allocate a new data block
 */
uint32_t allocate_data_block(void) {
    if (!superblock) return 0;
    
    for (uint32_t i = 0; i < superblock->data_block_count; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        
        if (!(superblock->block_bitmap[byte_index] & (1 << bit_index))) {
            // Mark as allocated
            superblock->block_bitmap[byte_index] |= (1 << bit_index);
            superblock->free_block_count--;
            return DATA_BLOCKS_START_SECTOR + i;
        }
    }
    return 0; // No free blocks
}

/**
 * Free an inode
 */
void free_inode(uint32_t inode_num) {
    if (!superblock || inode_num == 0 || inode_num >= MAX_DISK_INODES) return;
    
    uint32_t byte_index = inode_num / 8;
    uint32_t bit_index = inode_num % 8;
    
    superblock->inode_bitmap[byte_index] &= ~(1 << bit_index);
    superblock->free_inode_count++;
}

/**
 * Free a data block
 */
void free_data_block(uint32_t block_num) {
    if (!superblock || block_num < DATA_BLOCKS_START_SECTOR) return;
    
    uint32_t block_index = block_num - DATA_BLOCKS_START_SECTOR;
    if (block_index >= superblock->data_block_count) return;
    
    uint32_t byte_index = block_index / 8;
    uint32_t bit_index = block_index % 8;
    
    superblock->block_bitmap[byte_index] &= ~(1 << bit_index);
    superblock->free_block_count++;
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
 * Format a disk with MooseOS custom filesystem format
 */
int fs_format(uint8_t drive) {
    // Initialize superblock
    superblock = &sb_cache;
    superblock->signature = FILESYSTEM_SIGNATURE;
    superblock->total_sectors = 1000; // Assume 1000 sectors for now
    superblock->inode_count = MAX_DISK_INODES;
    superblock->free_inode_count = MAX_DISK_INODES - 1; // Root takes one
    superblock->data_block_count = 400; // 400 data blocks
    superblock->free_block_count = 400;
    superblock->root_inode = 1;
    
    // Clear bitmaps
    for (int i = 0; i < 64; i++) {
        superblock->inode_bitmap[i] = 0;
        superblock->block_bitmap[i] = 0;
    }
    
    // Mark root inode as allocated
    superblock->inode_bitmap[1 / 8] |= (1 << (1 % 8));
    
    // Write superblock to disk with bounds checking
    if (sizeof(superblock_t) > SECTOR_SIZE) {
        return -8; // Superblock too large for sector
    }
    
    // Clear disk buffer first
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    // Copy superblock 
    for (uint32_t i = 0; i < sizeof(superblock_t); i++) {
        disk_buffer[i] = ((uint8_t*)superblock)[i];
    }
    
    if (disk_write_sector(drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        return -1; // Failed to write superblock
    }
    
    // Clear inode table sectors
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    for (uint32_t sector = INODE_TABLE_SECTOR; sector < DATA_BLOCKS_START_SECTOR; sector++) {
        if (disk_write_sector(drive, sector, disk_buffer) != 0) {
            return -1; // Failed to clear inode table
        }
    }
    
    // Create root directory inode
    disk_inode_t root_inode;
    
    // Clear the structure first
    for (int i = 0; i < sizeof(disk_inode_t); i++) {
        ((uint8_t*)&root_inode)[i] = 0;
    }
    
    // Initialise
    root_inode.signature = FILESYSTEM_SIGNATURE;
    root_inode.inode_number = 1;
    root_inode.type = FOLDER_NODE;
    root_inode.size = 0;
    root_inode.parent_inode = 0;
    root_inode.child_count = 0;
    copyStr(root_inode.name, "/");
    
    for (int i = 0; i < 12; i++) {
        root_inode.data_blocks[i] = 0;
    }
    root_inode.indirect_block = 0;
    
    // Initialize child_inodes array
    for (int i = 0; i < MAX_CHILDREN_PER_DIR; i++) {
        root_inode.child_inodes[i] = 0;
    }
    
    if (write_inode_to_disk(1, &root_inode) != 0) {
        return -1; // Failed to write root inode
    }
    
    // Only set mounted flag if everything succeeded
    boot_drive = drive;
    filesystem_mounted = 1;
    
    return 0;
}

/**
 * Mount filesystem from disk
 */
int fs_mount(uint8_t drive) {
    // Read superblock
    if (disk_read_sector(drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        return -1;
    }
    
    // Copy superblock data
    superblock = &sb_cache;
    for (uint32_t i = 0; i < sizeof(superblock_t); i++) {
        ((uint8_t*)superblock)[i] = disk_buffer[i];
    }
    
    // Verify filesystem signature
    if (superblock->signature != FILESYSTEM_SIGNATURE) {
        return -2; // Invalid filesystem
    }
    
    boot_drive = drive;
    filesystem_mounted = 1;
    
    return 0;
}

/**
 * Sync filesystem to disk 
 */
int filesys_sync(void) {
    if (!filesystem_mounted || !superblock) return -1;
    
    // Write superblock to disk with bounds checking
    if (sizeof(superblock_t) > SECTOR_SIZE) {
        return -8; // Superblock too large for sector
    }
    
    // Clear disk buffer first
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    // Copy superblock
    for (uint32_t i = 0; i < sizeof(superblock_t); i++) {
        disk_buffer[i] = ((uint8_t*)superblock)[i];
    }
    
    if (disk_write_sector(boot_drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        return -1;
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
static int save_directory_recursive(File *dir, uint32_t dir_inode_num, uint32_t parent_inode) {
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
 * Save current filesystem to disk
 */
int fs_save_to_disk(void) {
    if (!filesystem_mounted || !superblock || !root) return -1;
    
    // Clear all inodes except root
    for (uint32_t i = 2; i < MAX_DISK_INODES; i++) {
        free_inode(i);
    }
    
    // Save the root directory and all its contents recursively
    if (save_directory_recursive(root, 1, 0) != 0) {
        return -1;
    }
    
    // Sync superblock
    return filesys_sync();
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
static int load_directory_recursive(uint32_t inode_num, File *parent) {
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

/**
 * Load filesystem from disk to memory
 */
int fs_load_from_disk(void) {
    if (!filesystem_mounted || !superblock) return -1;
    
    // Clear current in-memory filesystem and free all allocated files
    if (root) {
        file_free(root); // This will recursively free all children
        root = NULL;
        cwd = NULL;
    }
    fileCount = 0;
    
    // Load root directory
    disk_inode_t root_disk_inode;
    if (read_inode_from_disk(1, &root_disk_inode) != 0) {
        return -1;
    }
    
    if (root_disk_inode.signature != FILESYSTEM_SIGNATURE) {
        return -1;
    }
    
    // Create root
    root = file_alloc();
    if (!root) return -1;
    
    copyStr(root->name, "/");
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;
    cwd = root;
    
    // Load all children of root recursively
    for (uint32_t i = 0; i < root_disk_inode.child_count && i < MAX_CHILDREN_PER_DIR; i++) {
        uint32_t child_inode = root_disk_inode.child_inodes[i];
        if (child_inode > 0) {
            load_directory_recursive(child_inode, root);
        }
    }
    
    return 0;
}

/**
 * Get disk information as a formatted string
 * 
 * @note this is implemented wierdly. However, we must focus on other stuff
 * because such is life.
 * 
 * This is more of a debug thing.
 */
int fs_get_disk_info(char *info_buffer, int buffer_size) {
    if (!info_buffer || buffer_size < 200) return -1;
    
    char temp[100];
    info_buffer[0] = '\0';
    
    // Add disk drive information
    strcat(info_buffer, "Disk Info\n");
    
    for (int i = 0; i < 4; i++) {
        if (ata_devices[i].exists) {
            strcat(info_buffer, "Drive ");
            temp[0] = '0' + i;
            temp[1] = '\0';
            strcat(info_buffer, temp);
            strcat(info_buffer, ": ");
            strcat(info_buffer, ata_devices[i].model);
            strcat(info_buffer, "\n  Size: ");
            
            // Convert size to string
            uint32_t size_mb = (ata_devices[i].size * 512) / (1024 * 1024);
            int pos = 0;
            uint32_t temp_size = size_mb;
            if (temp_size == 0) {
                temp[pos++] = '0';
            } else {
                while (temp_size > 0) {
                    temp[pos++] = '0' + (temp_size % 10);
                    temp_size /= 10;
                }
                // Reverse the string
                for (int j = 0; j < pos / 2; j++) {
                    char swap = temp[j];
                    temp[j] = temp[pos - 1 - j];
                    temp[pos - 1 - j] = swap;
                }
            }
            temp[pos] = '\0';
            strcat(info_buffer, temp);
            strcat(info_buffer, " MB\n");
        }
    }
    
    // Add filesystem information
    if (filesystem_mounted && superblock) {
        strcat(info_buffer, "\nFilesystem status:\n");
        strcat(info_buffer, "Status: Mounted\n");
        strcat(info_buffer, "Free inodes: ");
        
        // Convert free inode count to string
        int pos = 0;
        uint32_t temp_count = superblock->free_inode_count;
        if (temp_count == 0) {
            temp[pos++] = '0';
        } else {
            while (temp_count > 0) {
                temp[pos++] = '0' + (temp_count % 10);
                temp_count /= 10;
            }
            // Reverse the string
            for (int j = 0; j < pos / 2; j++) {
                char swap = temp[j];
                temp[j] = temp[pos - 1 - j];
                temp[pos - 1 - j] = swap;
            }
        }
        temp[pos] = '\0';
        strcat(info_buffer, temp);
        strcat(info_buffer, "\n");
    } else {
        strcat(info_buffer, "\nFilesystem Status:\n");
        strcat(info_buffer, "Status: Not mounted\n");
    }
    
    return 0;
}

/**
 * Get disk status (1 = mounted, 0 = not mounted)
 */
int filesys_disk_status(void) {
    return filesystem_mounted;
}

/**
 * Flush filesystem cache to disk
 */
void filesys_flush_cache(void) {
    // Check if filesystem is mounted
    if (filesystem_mounted) {
        // First sync the superblock
        filesys_sync();
        
        // Then save all filesystem data
        fs_save_to_disk();
        
        // Force hardware cache flush
        disk_force_flush(boot_drive);
    }
}

/**
 * Get filesystem memory statistics
 * 
 * @note Yes this is also a debug thing. 
 *       It's pretty... wierd... but we won't fix it just yet.
 * 
 * This function is currently being used by the terminal
 */
int filesys_get_memory_stats(char *stats_buffer, int buffer_size) {
    if (!stats_buffer || buffer_size < 200) return -1;
    
    char temp[50];
    stats_buffer[0] = '\0';
    
    strcat(stats_buffer, "Filesystem Memory Statistics:\n");
    
    strcat(stats_buffer, "Active Files: ");
    int2str(fileCount, temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, "\n");
    
    // Calculate memory usage
    int total_memory = fileCount * sizeof(File); // Base structures
    int content_memory = 0;
    int children_memory = 0;

    /**
     * @todo Make this print the exact amount of bytes the disk is using.
     */
    strcat(stats_buffer, "Base Structures: ");
    int2str(total_memory, temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, " bytes\n");
    
    strcat(stats_buffer, "Per-File Base Size: ");
    int2str(sizeof(File), temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, " bytes\n");
    
    strcat(stats_buffer, "Note: + dynamic content/children\n");
    
    return 0;
}