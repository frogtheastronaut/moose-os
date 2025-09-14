#include "../include/filesystem.h"

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
 * Create directory
 * @return 0 on success, negative on failure
 */
int fs_make_dir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) return -2; // Invalid name - too long/empty
    if (name_in_CWD(name, FOLDER_NODE)) return -4; // Duplicate directory name

    File* node = file_alloc();
    if (!node) return -1;
    
    copyStr(node->name, name);
    node->type = FOLDER_NODE;
    
    // Initialize directory structure
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
