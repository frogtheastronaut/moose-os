#include "file/file_alloc.h"

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
