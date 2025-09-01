/**
    Moose Operating System
    Copyright (c) 2025 Ethan Zhang.

    @todo add disk I/O, dynamic file handling.
*/

// Includes
#include "file.h"

/** This is the file system
 *
 * @todo Remove limitations
 */
File filesys[MAX_NODES];
int fileCount = 0;

// Buffer used for multiple purposes
char buffer[256];

// Root and Current Working Director
File* root;
File* cwd;

// Disk filesystem globals
superblock_t *superblock = NULL;
uint8_t filesystem_mounted = 0;
uint8_t boot_drive = 0;
static uint8_t disk_buffer[SECTOR_SIZE];
static superblock_t sb_cache;

/**
 * Auto-save helper function - saves filesystem to disk after changes
 */
static void auto_save_filesystem(void) {
    if (filesystem_mounted && superblock) {
        filesys_sync();
        filesys_save_to_disk();
        filesys_flush_cache();
    }
}

/**
 * It's like Malloc, but for files.
 * Allocates a new File structure from a filesys
 * @return pointer to File, or NULL if full
 */
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

 /**
 * Check if file name in CWD
 * @return true if exists, false if not
 */
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

/** Make directory
 * @return number depending on success
 */
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

    // Auto-save to disk after creating directory
    auto_save_filesystem();

    // Success
    return 0;
}

/** Create file
 * @return number depending on success
 */
int filesys_mkfile(const char* name, const char* content) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name - too long/empty
    if (!content || strlen(content) >= MAX_CONTENT) return -3; // Content too large
    /** @todo remove limitations and add dynamic file sizes. */
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

    // Auto-save to disk after creating file
    auto_save_filesystem();

    // Success
    return 0;
}

/** Change directory
 * @return 0 on success, -1 on failure
 */
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

/** Remove file
 * @return 0 on success, -1 on failure
 */
int filesys_rm(const char* name) {
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, name)) {
            // Shift remaining children left
            for (int j = i; j < cwd->folder.childCount - 1; j++) {
                cwd->folder.children[j] = cwd->folder.children[j + 1];
            }
            cwd->folder.childCount--;
            
            // Auto-save to disk after removing file
            auto_save_filesystem();
            
            return 0;
        }
    }
    return -1; // Not found or not a file (is folder)
}

/** Remove folder
 * @return 0 on success, -1 on failure, -2 if not empty
 */
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
            
            // Auto-save to disk after removing directory
            auto_save_filesystem();
            
            return 0;
        }
    }
    return -1; // Not found/not a directory (is a file)
}

/** Edit file content
 * @return 0 on success, -1 on failure
 */
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
            
            // Auto-save to disk after editing file
            auto_save_filesystem();
            
            return 0;
        }
    }
    return -1;
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
 * Format a disk with MooseOS filesystem
 */
int filesys_format(uint8_t drive) {
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
    
    // Copy superblock safely
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
int filesys_mount(uint8_t drive) {
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
 * Sync filesystem to disk (write superblock)
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
    
    // Copy superblock safely
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
static int convert_memory_to_disk_inode(File *memory_file, disk_inode_t *disk_inode, uint32_t inode_num, uint32_t parent_inode) {
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
        disk_inode->size = strlen(memory_file->file.content);
        
        // Write content to data blocks if file has content
        if (disk_inode->size > 0) {
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
 */
static int save_directory_recursive(File *dir, uint32_t dir_inode_num, uint32_t parent_inode) {
    if (!dir || dir->type != FOLDER_NODE) return -1;
    
    disk_inode_t dir_disk_inode;
    if (convert_memory_to_disk_inode(dir, &dir_disk_inode, dir_inode_num, parent_inode) != 0) {
        return -1;
    }
    
    // Save directory inode
    if (write_inode_to_disk(dir_inode_num, &dir_disk_inode) != 0) {
        return -1;
    }
    
    // Save children
    for (int i = 0; i < dir->folder.childCount && i < MAX_CHILDREN_PER_DIR; i++) {
        File *child = dir->folder.children[i];
        uint32_t child_inode = allocate_inode();
        if (child_inode == 0) continue;
        
        if (child->type == FILE_NODE) {
            disk_inode_t child_disk_inode;
            if (convert_memory_to_disk_inode(child, &child_disk_inode, child_inode, dir_inode_num) == 0) {
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
 * Save current in-memory filesystem to disk
 */
int filesys_save_to_disk(void) {
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
    
    File *memory_file = allocFile();
    if (!memory_file) return NULL;
    
    // Copy basic information
    copyStr(memory_file->name, disk_inode->name);
    memory_file->type = disk_inode->type;
    
    if (disk_inode->type == FILE_NODE) {
        // Load file content from disk blocks
        if (disk_inode->size > 0 && disk_inode->data_blocks[0] > 0) {
            uint8_t content_buffer[SECTOR_SIZE];
            
            if (disk_read_sector(boot_drive, disk_inode->data_blocks[0], content_buffer) == 0) {
                // Copy content to memory file
                uint32_t content_len = disk_inode->size;
                if (content_len >= MAX_CONTENT) {
                    content_len = MAX_CONTENT - 1; // Prevent overflow
                }
                
                for (uint32_t i = 0; i < content_len; i++) {
                    memory_file->file.content[i] = content_buffer[i];
                }
                memory_file->file.content[content_len] = '\0';
            } else {
                // Failed to read content
                memory_file->file.content[0] = '\0';
            }
        } else {
            memory_file->file.content[0] = '\0';
        }
    } else {
        // For directories
        memory_file->folder.childCount = 0;
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
    
    if (parent && parent->type == FOLDER_NODE && parent->folder.childCount < MAX_CHILDREN) {
        parent->folder.children[parent->folder.childCount++] = memory_file;
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
int filesys_load_from_disk(void) {
    if (!filesystem_mounted || !superblock) return -1;
    
    // Clear current in-memory filesystem
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
    root = allocFile();
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
 */
int filesys_get_disk_info(char *info_buffer, int buffer_size) {
    if (!info_buffer || buffer_size < 200) return -1;
    
    char temp[100];
    info_buffer[0] = '\0';
    
    // Add disk drive information
    strcat(info_buffer, "=== MooseOS Disk Information ===\n");
    
    for (int i = 0; i < 4; i++) {
        if (ata_devices[i].exists) {
            strcat(info_buffer, "Drive ");
            temp[0] = '0' + i;
            temp[1] = '\0';
            strcat(info_buffer, temp);
            strcat(info_buffer, ": ");
            strcat(info_buffer, ata_devices[i].model);
            strcat(info_buffer, "\n  Size: ");
            
            // Convert size to string (simplified)
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
        strcat(info_buffer, "\n=== Filesystem Status ===\n");
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
        strcat(info_buffer, "\n=== Filesystem Status ===\n");
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
 * Flush filesystem cache to disk with additional verification
 */
void filesys_flush_cache(void) {
    if (filesystem_mounted) {
        // First sync the superblock
        filesys_sync();
        
        // Then save all filesystem data
        filesys_save_to_disk();
        
        // Force hardware cache flush using the dedicated function
        disk_force_flush(boot_drive);
    }
}