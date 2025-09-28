/*
    MooseOS Filesystem code
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/
#include "filesystem/filesystem.h"
#include "print/debug.h"

/**
 * autosaves filesystem to disk after changes
 */
static void auto_save_filesystem(void) {
    if (filesystem_mounted && superblock) {
        filesystem_sync();
        filesystem_save_to_disk();
        filesystem_flush_cache();
    } else {
        debugf("[FS] Filesystem not mounted, cannot auto-save\n");
    }
}

// initialise filesystem.
void filesystem_init() {
    root = file_alloc();
    if (!root) {
        // root does not exist (file_alloc failed)
        debugf("[FS] Failed to allocate root directory\n");
        return;
    }
    copyStr(root->name, "/"); // root is /

    // initialise root
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.children = NULL;
    root->folder.childCount = 0;
    root->folder.capacity = 0;

    // we will start at root
    cwd = root;
}

/** 
 * create file
 * @return number depending on success
 */
int filesystem_make_file(const char* name, const char* content) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) {
        debugf("[FS] Invalid file name - too long/empty\n");
        return -2; // invalid name - too long/empty
    }
    if (!content) {
        debugf("[FS] Invalid file content - NULL\n");
        return -3; // content is NULL
    }
    if (name_in_cwd(name, FILE_NODE)) {
        debugf("[FS] Duplicate file name\n");
        return -4; // duplicate file name
    }

    File* node = file_alloc();
    if (!node) {
        debugf("[FS] Failed to allocate file node\n");
        return -1;
    }
    
    copyStr(node->name, name);
    node->type = FILE_NODE;
    
    // set content
    if (set_file_content(node, content) != 0) {
        file_free(node);
        debugf("[FS] Failed to allocate file content\n");
        return -1; // failed to allocate content
    }
    
    // add to current directory
    if (add_child_to_dir(cwd, node) != 0) {
        file_free(node);
        debugf("[FS] Failed to add file to current directory\n");
        return -1;
    }

    // auto-save to disk after creating file
    auto_save_filesystem();

    return 0;
}

/**
 * create directory
 * @return 0 on success, negative on failure
 */
int filesystem_make_dir(const char* name) {
    if (!name || strlen(name) == 0 || strlen(name) >= MAX_NAME_LEN) {
        debugf("[FS] Invalid directory name - too long/empty\n");
        return -2; // invalid name - too long/empty
    }
    if (name_in_cwd(name, FOLDER_NODE)) {
        debugf("[FS] Duplicate directory name\n");
        return -4; // duplicate directory name
    }

    File* node = file_alloc();
    if (!node) {
        debugf("[FS] Failed to allocate directory node\n");
        return -1;
    }
    
    copyStr(node->name, name);
    node->type = FOLDER_NODE;
    
    // initialize directory structure
    node->folder.children = NULL;
    node->folder.childCount = 0;
    node->folder.capacity = 0;
    
    // add to current directory
    if (add_child_to_dir(cwd, node) != 0) {
        file_free(node);
        debugf("[FS] Failed to add directory to current directory\n");
        return -1;
    }

    // auto-save to disk after creating directory
    auto_save_filesystem();

    return 0; // success
}

/** 
 * change directory
 * @return 0 on success, -1 on failure
 */
int filesystem_change_dir(const char* name) {
    // .. means to go back to parent folder
    if (strEqual(name, "..")) {
        if (cwd->parent != NULL) cwd = cwd->parent;
        return 0;
    }
    
    if (!cwd->folder.children) { // no children
        debugf("[FS] No children in current directory\n");
        return -1; 
    }
    
    // change cwd to folder
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FOLDER_NODE && strEqual(child->name, name)) {
            cwd = child;
            return 0;
        }
    }
    debugf("[FS] Directory not found\n");
    return -1; // not found
}

/** 
 * remove file
 * @return 0 on success, -1 on failure
 */
int filesystem_remove(const char* name) {
    if (!cwd->folder.children) {
        debugf("[FS] No children in current directory\n");
        return -1;
    }
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FILE_NODE && strEqual(child->name, name)) {
            // remove from directory
            if (remove_child_from_dir(cwd, child) == 0) {
                // free the file memory
                file_free(child);
                auto_save_filesystem();
                return 0;
            }
        }
    }
    debugf("[FS] File not found/is folder\n");
    return -1; // not found or not a file (is folder)
}


/** 
 * remove folder
 * @return 0 on success, -1 on failure, -2 if not empty
 */
int filesystem_remove_dir(const char* name) {
    if (!cwd->folder.children) {
        debugf("[FS] No children in current directory\n");
        return -1;
    }
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FOLDER_NODE && strEqual(child->name, name)) {
            if (child->folder.childCount > 0) {
                debugf("[FS] Directory not empty\n");
                return -2; // directory not empty
            }

            // remove from directory
            if (remove_child_from_dir(cwd, child) == 0) {
                // free the directory memory
                file_free(child);
                auto_save_filesystem();
                
                return 0;
            }
        }
    }
    debugf("[FS] Directory not found/is file\n");
    return -1; // not found/not a directory (is a file)
}

/** 
 * edit file content
 * @return 0 on success, -1 on failure
 */
int filesystem_edit_file(const char* name, const char* new_content) {
    if (!cwd->folder.children) {
        debugf("[FS] No children in current directory\n");
        return -1; // No children
    }
    
    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == FILE_NODE && strEqual(child->name, name)) {
            // set new content
            if (set_file_content(child, new_content) == 0) {
                // successfully set content
                auto_save_filesystem();
                return 0;
            }
            debugf("[FS] Failed to set file content\n");
            return -1; // failed to set content
        }
    }
    debugf("[FS] File not found/is folder\n");
    return -1; // file not found
}


/**
 * format a disk with MooseOS format
 */
int filesystem_format(uint8_t drive) {
    // initialize superblock
    superblock = &sb_cache;
    superblock->signature = FILESYSTEM_SIGNATURE;
    superblock->total_sectors = 1000; // 1000 sectors for now
    superblock->inode_count = MAX_DISK_INODES;
    superblock->free_inode_count = MAX_DISK_INODES - 1; // Root takes 1
    superblock->data_block_count = 400; // 400 data blocks
    superblock->free_block_count = 400;
    superblock->root_inode = 1;
    
    // clear bitmaps
    for (int i = 0; i < 64; i++) {
        superblock->inode_bitmap[i] = 0;
        superblock->block_bitmap[i] = 0;
    }

    // mark root inode as allocated
    superblock->inode_bitmap[1 / 8] |= (1 << (1 % 8));
    
    // write superblock to disk
    if (sizeof(file_superblock) > SECTOR_SIZE) {
        debugf("[FS] Superblock too large for sector\n");
        return -8; // superblock too large for sector
    }
    
    // clear disk buffer first
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    // copy superblock 
    for (uint32_t i = 0; i < sizeof(file_superblock); i++) {
        disk_buffer[i] = ((uint8_t*)superblock)[i];
    }
    
    if (disk_write_sector(drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        debugf("[FS] Failed to write superblock to disk\n");
        return -1; // failed to write superblock
    }

    // clear inode table sectors
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    for (uint32_t sector = INODE_TABLE_SECTOR; sector < DATA_BLOCKS_START_SECTOR; sector++) {
        if (disk_write_sector(drive, sector, disk_buffer) != 0) {
            debugf("[FS] Failed to clear inode table on disk\n");
            return -1; // failed to clear inode table
        }
    }
    
    // create root directory inode
    disk_inode root_inode;
    
    // clear the structure first
    for (int i = 0; i < sizeof(disk_inode); i++) {
        ((uint8_t*)&root_inode)[i] = 0;
    }

    // initialise root inode
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
    
    // initialize child_inodes array
    for (int i = 0; i < MAX_CHILDREN_PER_DIR; i++) {
        root_inode.child_inodes[i] = 0;
    }
    
    if (write_inode_to_disk(1, &root_inode) != 0) {
        debugf("[FS] Failed to write root inode to disk\n");
        return -1; // failed to write root inode
    }
    
    // everything has succeeded :thumbsup:
    boot_drive = drive;
    filesystem_mounted = 1;
    return 0;
}

/**
 * mount filesystem from disk
 */
int filesystem_mount(uint8_t drive) {
    // read superblock
    if (disk_read_sector(drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        debugf("[FS] Failed to read superblock from disk\n");
        return -1;
    }
    
    // copy superblock data
    superblock = &sb_cache;
    for (uint32_t i = 0; i < sizeof(file_superblock); i++) {
        ((uint8_t*)superblock)[i] = disk_buffer[i];
    }
    
    // verify filesystem signature
    if (superblock->signature != FILESYSTEM_SIGNATURE) {
        debugf("[FS] Filesystem signature mismatch\n");
        return -2; // signature mismatch
    }
    
    // everything has succeeded
    boot_drive = drive;
    filesystem_mounted = 1;
    return 0;
}

/**
 * sync filesystem to disk 
 */
int filesystem_sync(void) {
    if (!filesystem_mounted || !superblock) {
        debugf("[FS] Filesystem not mounted or superblock missing\n");
        return -1;
    }
    
    // write superblock to disk
    if (sizeof(file_superblock) > SECTOR_SIZE) {
        debugf("[FS] Superblock too large for sector\n");
        return -8; // superblock too large for sector
    }

    // clear disk buffer first
    for (int i = 0; i < SECTOR_SIZE; i++) {
        disk_buffer[i] = 0;
    }
    
    // copy superblock
    for (uint32_t i = 0; i < sizeof(file_superblock); i++) {
        disk_buffer[i] = ((uint8_t*)superblock)[i];
    }
    
    if (disk_write_sector(boot_drive, SUPERBLOCK_SECTOR, disk_buffer) != 0) {
        debugf("[FS] Failed to write superblock to disk\n");
        return -1;
    }
    
    return 0; // success
}

/**
 * save current filesystem to disk
 */
int filesystem_save_to_disk(void) {
    if (!filesystem_mounted || !superblock || !root) {
        debugf("[FS] Filesystem not mounted, superblock or root missing\n");
        return -1;
    }
    
    // clear all inodes except root
    for (uint32_t i = 2; i < MAX_DISK_INODES; i++) {
        free_inode(i);
    }
    
    // save the root directory and all its contents recursively
    if (save_directory_recursive(root, 1, 0) != 0) {
        debugf("[FS] Failed to save directory recursively\n");
        return -1;
    }
    
    // sync superblock
    return filesystem_sync();
}


/**
 * load filesystem from disk to memory
 */
int filesystem_load_from_disk(void) {
    if (!filesystem_mounted || !superblock) {
        debugf("[FS] Filesystem not mounted or superblock missing\n");
        return -1;
    }

    // clear current in-memory filesystem and free all allocated files
    if (root) {
        file_free(root); // this will recursively free all children
        root = NULL;
        cwd = NULL;
    }
    file_count = 0;
    
    // load root directory
    disk_inode root_disk_inode;
    if (read_inode_from_disk(1, &root_disk_inode) != 0) {
        debugf("[FS] Failed to read root inode from disk\n");
        return -1;
    }
    
    if (root_disk_inode.signature != FILESYSTEM_SIGNATURE) {
        debugf("[FS] Root inode signature mismatch\n");
        return -1;
    }
    
    // create root
    root = file_alloc();
    if (!root) return -1;
    
    copyStr(root->name, "/");
    root->type = FOLDER_NODE;
    root->parent = NULL;
    root->folder.childCount = 0;
    cwd = root;

    // load all children of root recursively
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
 * @note This is more of a debug thing.
 */
int filesystem_get_disk_info(char *info_buffer, int buffer_size) {
    if (!info_buffer || buffer_size < 200) return -1;
    
    char temp[100];
    info_buffer[0] = '\0';
    
    // add disk drive information
    strcat(info_buffer, "Disk Info\n");
    
    /**
     * yes this is wierd
     * if you get confused looking at this, make sure to submit a pull request on Github
     * that increases this counter
     * 
     * confused_coders = 1;
     */
    for (int i = 0; i < 4; i++) {
        if (ata_devices[i].exists) {
            strcat(info_buffer, "Drive ");
            temp[0] = '0' + i;
            temp[1] = '\0';
            strcat(info_buffer, temp);
            strcat(info_buffer, ": ");
            strcat(info_buffer, ata_devices[i].model);
            strcat(info_buffer, "\n  Size: ");
            
            // convert size to string
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
                // reverse the string
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
    
    // add filesystem information
    if (filesystem_mounted && superblock) {
        strcat(info_buffer, "\nFilesystem status:\n");
        strcat(info_buffer, "Status: Mounted\n");
        strcat(info_buffer, "Free inodes: ");

        // convert free inode count to string
        int pos = 0;
        uint32_t temp_count = superblock->free_inode_count;
        if (temp_count == 0) {
            temp[pos++] = '0';
        } else {
            while (temp_count > 0) {
                temp[pos++] = '0' + (temp_count % 10);
                temp_count /= 10;
            }
            // reverse the string
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

    return 0; // success, but the way the code is written makes me want to think it'll fail
}

/**
 * get disk status 
 * @returns 1 if mounted, 0 if not
 */
int filesystem_disk_status(void) {
    return filesystem_mounted;
}

/**
 * flush filesystem cache to disk
 */
void filesystem_flush_cache(void) {
    // check if filesystem is mounted
    if (filesystem_mounted) {
        filesystem_sync();
        filesystem_save_to_disk();
        
        /**
         * @todo are we sure we want to do this?
         * this may ruin the physical disk
         */
        disk_force_flush(boot_drive);
    }
}

/**
 * get filesystem memory statistics
 *
 * @note yes this is also a debug thing.
 *       it's pretty... wierd... but we won't fix it just yet.
 * here is another counter:
 * confused_coders = 1;
 * 
 * @note this function is currently being used by the terminal
 */
int filesystem_get_memory_stats(char *stats_buffer, int buffer_size) {
    if (!stats_buffer || buffer_size < 200) return -1;
    
    char temp[50];
    stats_buffer[0] = '\0';
    
    strcat(stats_buffer, "Filesystem Memory Statistics:\n");
    
    strcat(stats_buffer, "Active Files: ");
    int2str(file_count, temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, "\n");
    
    // calculate memory usage
    int total_memory = file_count * sizeof(File); // base structures
    int content_memory = 0;
    int children_memory = 0;

    /**
     * @todo make this print the exact amount of bytes the disk is using.
     */
    strcat(stats_buffer, "Base Structures: ");
    int2str(total_memory, temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, " bytes\n");
    
    strcat(stats_buffer, "Per-File Base Size: ");
    int2str(sizeof(File), temp, sizeof(temp));
    strcat(stats_buffer, temp);
    strcat(stats_buffer, " bytes\n");
    
    return 0; // success, but the way the code is written makes me want to think it'll fail
}

char* get_file_content(const char* filename) {
    if (!cwd->folder.children) {
        debugf("[FS] No children in current directory\n");
        return NULL;
    }
    for (int i = 0; i < cwd->folder.childCount; i++) {
        if (!cwd->folder.children[i]) continue;
        
        File* child = cwd->folder.children[i];
        if (child->type == FILE_NODE && strEqual(child->name, filename)) {
            return child->file.content;
        }
    }
    debugf("[FS] File not found/is folder\n");
    return NULL;
}
