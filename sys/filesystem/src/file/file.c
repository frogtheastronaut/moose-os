/*
    MooseOS File Allocation code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "file/file.h"
#include "file/file_alloc.h"
#include "print/debug.h"

// current file count
int file_count = 0;

// buffer - used for multiple purposes
char buffer[256];

// root and current working directory
File* root;
File* cwd;

// superblock
file_superblock *superblock = NULL;

// 1 if filesystem is mounted, 0 if not
// currently, it's not mounted
uint8_t filesystem_mounted = 0;

// currently, boot drive does not exist
uint8_t boot_drive = 0;

// disk buffer of sector size
uint8_t disk_buffer[SECTOR_SIZE];

// cache
file_superblock sb_cache;

/**
 * set file content
 */
int set_file_content(File* file, const char* content) {
    if (!file || file->type != FILE_NODE || !content) {
        debugf("[FILE] Invalid file or content\n");
        return -1;
    }
    
    size_t new_size = strlen(content);
    
    // free old content
    if (file->file.content) {
        kfree(file->file.content);
    }
    
    // allocate new content
    file->file.content = (char*)kmalloc(new_size + 1);
    if (!file->file.content) {
        file->file.content_size = 0;
        file->file.content_capacity = 0;
        debugf("[FILE] Out of memory allocating file content\n");
        return -1; // out of memory
    }
    
    // copy content over
    for (size_t i = 0; i < new_size; i++) {
        file->file.content[i] = content[i];
    }
    file->file.content[new_size] = '\0';
    
    file->file.content_size = new_size;
    file->file.content_capacity = new_size + 1;
    
    return 0; // success
}

/**
 * set file content with binary data
 * @param file the file to set content for
 * @param data binary data pointer
 * @param size size of the binary data
 * @returns 0 on success, -1 on failure.
 */
int set_file_content_binary(File* file, const char* data, size_t size) {
    if (!file || file->type != FILE_NODE || !data) {
        debugf("[FILE] Invalid file or data\n");
        return -1;
    }
    
    // free old content
    if (file->file.content) {
        kfree(file->file.content);
    }
    
    // allocate new content (no null terminator needed for binary)
    file->file.content = (char*)kmalloc(size);
    if (!file->file.content) {
        file->file.content_size = 0;
        file->file.content_capacity = 0;
        debugf("[FILE] Out of memory allocating file content\n");
        return -1; // out of memory
    }
    
    // copy binary data over
    for (size_t i = 0; i < size; i++) {
        file->file.content[i] = data[i];
    }
    
    file->file.content_size = size;
    file->file.content_capacity = size;
    
    return 0; // success
}

/**
 * add child to directory
 * @returns 0 on success, -1 on failure.
 */
int add_child_to_dir(File* dir, File* child) {
    if (!dir || !child || dir->type != FOLDER_NODE) {
        debugf("[FILE] Invalid directory or child node \n");
        return -1;
    }

    // initialize children array if first child
    if (!dir->folder.children) {
        /**
         * @todo we could make this bigger
         */
        dir->folder.capacity = 4; 
        dir->folder.children = (File**)kmalloc(dir->folder.capacity * sizeof(File*));
        if (!dir->folder.children) {
            debugf("[FILE] Out of memory allocating children node array\n");
            return -1;
        }
        dir->folder.childCount = 0;
    }
    
    // grow array if needed
    if (dir->folder.childCount >= dir->folder.capacity) {
        int new_capacity = dir->folder.capacity * 2;
        File** new_children = (File**)krealloc(dir->folder.children, new_capacity * sizeof(File*));
        if (!new_children) {
            debugf("[FILE] Out of memory reallocating children node array\n");
            return -1;
        }

        dir->folder.children = new_children;
        dir->folder.capacity = new_capacity;
    }
    
    // add child
    dir->folder.children[dir->folder.childCount] = child;
    dir->folder.childCount++;
    child->parent = dir;
    
    return 0; // success
}

/**
 * remove child from directory
 * @return 0 on success, -1 on failure
 */
int remove_child_from_dir(File* dir, File* child) {
    if (!dir || !child || dir->type != FOLDER_NODE || !dir->folder.children) {
        debugf("[FILE] Invalid directory or child node\n");
        return -1;
    }

    // find child index
    int child_index = -1;
    for (int i = 0; i < dir->folder.childCount; i++) {
        if (dir->folder.children[i] == child) {
            child_index = i;
            break;
        }
    }

    if (child_index == -1) {
        debugf("[FILE] Child node not found in directory\n");
        return -1; // child not found
    }

    // shift remaining children left
    for (int i = child_index; i < dir->folder.childCount - 1; i++) {
        dir->folder.children[i] = dir->folder.children[i + 1];
    }
    dir->folder.childCount--;
    
    return 0;
}

 /**
 * check if file name in CWD
 * @return true if exists, false if not
 */
bool name_in_cwd(const char* name, file_node type) {
    if (!cwd || cwd->type != FOLDER_NODE || !cwd->folder.children) {
        return false;
    }

    for (int i = 0; i < cwd->folder.childCount; i++) {
        File* child = cwd->folder.children[i];
        if (child && child->type == type && strcmp(child->name, name)) {
            return true;
        }
    }
    return false;
}

/**
 * write inode to disk
 */
int write_inode_to_disk(uint32_t inode_num, disk_inode *inode) {
    if (inode_num == 0 || inode_num >= MAX_DISK_INODES) {
        debugf("[FILE] Invalid inode number\n");
        return -1;
    }
    if (!inode) {
        debugf("[FILE] Invalid inode pointer\n");
        return -1;
    }

    uint32_t sector = INODE_TABLE_SECTOR + (inode_num / INODES_PER_SECTOR);
    uint32_t inode_offset_in_sector = inode_num % INODES_PER_SECTOR;
    uint32_t byte_offset = inode_offset_in_sector * sizeof(disk_inode);
    
    // validate that inode fits in sector
    if (byte_offset + sizeof(disk_inode) > SECTOR_SIZE) {
        debugf("[FILE] Inode does not fit in sector\n");
        return -7; // inode doesn't fit in sector
    }
    
    // read sector, modify, write back
    if (disk_read_sector(boot_drive, sector, disk_buffer) != 0) {
        debugf("[FILE] Failed to read sector from disk\n");
        return -1;
    }
    
    // copy inode data
    uint8_t *src = (uint8_t*)inode;
    uint8_t *dst = disk_buffer + byte_offset;
    for (uint32_t i = 0; i < sizeof(disk_inode); i++) {
        dst[i] = src[i];
    }
    
    if (disk_write_sector(boot_drive, sector, disk_buffer) != 0) {
        debugf("[FILE] Failed to write sector to disk\n");
        return -1;
    }
    
    return 0; // success
}

/**
 * read inode from disk
 */
int read_inode_from_disk(uint32_t inode_num, disk_inode *inode) {
    if (inode_num == 0 || inode_num >= MAX_DISK_INODES) {
        debugf("[FILE] Invalid inode number\n");
        return -1;
    }
    if (!inode) {
        debugf("[FILE] Invalid inode pointer\n");
        return -1;
    }

    uint32_t sector = INODE_TABLE_SECTOR + (inode_num / INODES_PER_SECTOR);
    uint32_t inode_offset_in_sector = inode_num % INODES_PER_SECTOR;
    uint32_t byte_offset = inode_offset_in_sector * sizeof(disk_inode);
    
    // validate that inode fits in sector
    if (byte_offset + sizeof(disk_inode) > SECTOR_SIZE) {
        debugf("[FILE] Inode doesn't fit in sector\n");
        return -7; // inode doesn't fit in sector
    }
    
    if (disk_read_sector(boot_drive, sector, disk_buffer) != 0) {
        debugf("[FILE] Failed to read sector from disk\n");
        return -1;
    }
    
    // copy inode data
    uint8_t *src = disk_buffer + byte_offset;
    uint8_t *dst = (uint8_t*)inode;
    for (uint32_t i = 0; i < sizeof(disk_inode); i++) {
        dst[i] = src[i];
    }
    
    return 0; // success
}

/**
 * convert a memory File structure to disk inode format
 */
static int memory_to_inode(File *memory_file, disk_inode *disk_inode, uint32_t inode_num, uint32_t parent_inode) {
    if (!memory_file || !disk_inode) {
        debugf("[FILE] Invalid memory file or disk inode pointer\n");
        return -1;
    }

    // clear the disk inode
    for (uint32_t i = 0; i < sizeof(disk_inode); i++) {
        ((uint8_t*)disk_inode)[i] = 0;
    }
    
    disk_inode->signature = FILESYSTEM_SIGNATURE;
    disk_inode->inode_number = inode_num;
    disk_inode->type = memory_file->type;
    disk_inode->parent_inode = parent_inode;
    
    // copy name
    int name_len = 0;
    while (name_len < MAX_NAME_LEN - 1 && memory_file->name[name_len] != '\0') {
        disk_inode->name[name_len] = memory_file->name[name_len];
        name_len++;
    }
    disk_inode->name[name_len] = '\0';
    
    // handle file nodes
    if (memory_file->type == FILE_NODE) {
        // handle file content
        if (memory_file->file.content) {
            disk_inode->size = memory_file->file.content_size;
        } else {
            disk_inode->size = 0;
        }

        // write content to data blocks (if file has content)
        if (disk_inode->size > 0 && memory_file->file.content) {
            uint32_t data_block = allocate_data_block();
            if (data_block > 0) {
                disk_inode->data_blocks[0] = data_block;

                // write file content to disk block
                uint8_t content_buffer[SECTOR_SIZE];
                for (int i = 0; i < SECTOR_SIZE; i++) {
                    content_buffer[i] = 0; // clear buffer
                }
                
                // copy content
                uint32_t content_len = disk_inode->size;
                if (content_len > SECTOR_SIZE - 1) {
                    content_len = SECTOR_SIZE - 1; // leave space for null terminator
                }
                
                for (uint32_t i = 0; i < content_len; i++) {
                    content_buffer[i] = memory_file->file.content[i];
                }
                
                // write content to disk
                if (disk_write_sector(boot_drive, data_block, content_buffer) != 0) {
                    // failed to write content, free the block
                    debugf("[FILE] Failed to write file content to disk block\n");
                    debugf("[FILE] Freeing allocated data block...");
                    free_data_block(data_block);
                    disk_inode->data_blocks[0] = 0;
                    debugf("done\n");
                }
            }
        }
    } else {
        // handle directory
        disk_inode->size = memory_file->folder.childCount;
        disk_inode->child_count = 0; // will be filled by recursive save
    }
    
    return 0;
}

/**
 * recursively save directory tree to disk
 */
int save_directory_recursive(File *dir, uint32_t dir_inode_num, uint32_t parent_inode) {
    if (!dir || dir->type != FOLDER_NODE) {
        debugf("[FILE] Invalid directory node\n");
        return -1;
    }

    disk_inode dir_disk_inode;
    if (memory_to_inode(dir, &dir_disk_inode, dir_inode_num, parent_inode) != 0) {
        debugf("[FILE] Failed to convert directory to disk inode\n");
        return -1;
    }
    
    // save directory inode
    if (write_inode_to_disk(dir_inode_num, &dir_disk_inode) != 0) {
        debugf("[FILE] Failed to write directory inode to disk\n");
        return -1;
    }
    
    // save children
    for (int i = 0; i < dir->folder.childCount && i < MAX_CHILDREN_PER_DIR; i++) {
        if (!dir->folder.children || !dir->folder.children[i]) continue;
        
        File *child = dir->folder.children[i];
        uint32_t child_inode = allocate_inode();
        if (child_inode == 0) continue;
        
        if (child->type == FILE_NODE) {
            disk_inode child_disk_inode;
            if (memory_to_inode(child, &child_disk_inode, child_inode, dir_inode_num) == 0) {
                write_inode_to_disk(child_inode, &child_disk_inode);
                dir_disk_inode.child_inodes[dir_disk_inode.child_count++] = child_inode;
            }
        } else {
            // recursively save subdirectory
            if (save_directory_recursive(child, child_inode, dir_inode_num) == 0) {
                dir_disk_inode.child_inodes[dir_disk_inode.child_count++] = child_inode;
            }
        }
    }
    
    // update directory with child references
    write_inode_to_disk(dir_inode_num, &dir_disk_inode);
    
    return 0;
}

/**
 * convert disk inode to memory File structure
 */
static File* convert_disk_to_memory_file(disk_inode *disk_inode) {
    if (!disk_inode || disk_inode->signature != FILESYSTEM_SIGNATURE) {
        debugf("[FILE] Invalid disk inode or signature mismatch\n");
        return NULL;
    }

    File *memory_file = file_alloc();
    if (!memory_file) {
        debugf("[FILE] Failed to allocate memory for file structure\n");
        return NULL;
    }

    // copy basic information
    strcpy(memory_file->name, disk_inode->name);
    memory_file->type = disk_inode->type;
    
    if (disk_inode->type == FILE_NODE) {
        // load file content from disk blocks
        if (disk_inode->size > 0 && disk_inode->data_blocks[0] > 0) {
            uint8_t content_buffer[SECTOR_SIZE];
            
            if (disk_read_sector(boot_drive, disk_inode->data_blocks[0], content_buffer) == 0) {
                // allocate content
                memory_file->file.content = (char*)kmalloc(disk_inode->size + 1);
                if (memory_file->file.content) {
                    // copy content to memory file
                    uint32_t content_len = disk_inode->size;
                    if (content_len > SECTOR_SIZE - 1) { // prevent buffer overflow
                        content_len = SECTOR_SIZE - 1; 
                    }
                    
                    for (uint32_t i = 0; i < content_len; i++) {
                        memory_file->file.content[i] = content_buffer[i];
                    }
                    memory_file->file.content[content_len] = '\0';
                    memory_file->file.content_size = content_len;
                    memory_file->file.content_capacity = disk_inode->size + 1;
                } else {
                    // failed to allocate content memory
                    debugf("[FILE] Out of memory allocating file content\n");
                    memory_file->file.content = NULL;
                    memory_file->file.content_size = 0;
                    memory_file->file.content_capacity = 0;
                }
            } else {
                // failed to read content
                debugf("[FILE] Failed to read file content from disk block\n");
                memory_file->file.content = NULL;
                memory_file->file.content_size = 0;
                memory_file->file.content_capacity = 0;
            }
        } else {
            // empty file
            debugf("[FILE] File has no content\n");
            memory_file->file.content = NULL;
            memory_file->file.content_size = 0;
            memory_file->file.content_capacity = 0;
        }
    } else {
        // initialise empty
        debugf("[FILE] Empty directory\n");
        memory_file->folder.children = NULL;
        memory_file->folder.childCount = 0;
        memory_file->folder.capacity = 0;
    }
    
    return memory_file;
}

/**
 * recursively load directory tree from disk
 */
int load_directory_recursive(uint32_t inode_num, File *parent) {
    disk_inode disk_inode;
    if (read_inode_from_disk(inode_num, &disk_inode) != 0) {
        debugf("[FILE] Failed to read inode from disk\n");
        return -1;
    }
    
    if (disk_inode.signature != FILESYSTEM_SIGNATURE) {
        debugf("[FILE] Signature mismatch\n");
        return -1;
    }
    
    File *memory_file = convert_disk_to_memory_file(&disk_inode);
    if (!memory_file) {
        debugf("[FILE] Failed to convert disk inode to memory file\n");
        return -1;
    }

    memory_file->parent = parent;
    
    // add to parent directory if parent exists
    if (parent && parent->type == FOLDER_NODE) {
        if (add_child_to_dir(parent, memory_file) != 0) {
            file_free(memory_file);
            debugf("[FILE] Failed to add child to parent directory\n");
            return -1;
        }
    }

    // if this is a directory, load its children
    if (disk_inode.type == FOLDER_NODE) {
        for (uint32_t i = 0; i < disk_inode.child_count && i < MAX_CHILDREN_PER_DIR; i++) {
            uint32_t child_inode = disk_inode.child_inodes[i];
            if (child_inode > 0) {
                // recursively load because there's more than 1 child
                load_directory_recursive(child_inode, memory_file);
            }
        }
    }
    
    return 0; // success
}
