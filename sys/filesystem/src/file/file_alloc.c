/*
    MooseOS File Allocation code
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/

#include "file/file_alloc.h"
#include "print/debug.h"

/**
 * file allocator
 * @return pointer to File, or NULL if out of memory
 */
File* file_alloc() {
    File* new_file = (File*)kmalloc(sizeof(File));
    if (new_file) {
        // clear the allocated memory
        uint8_t* file_ptr = (uint8_t*)new_file;
        for (int i = 0; i < sizeof(File); i++) {
            file_ptr[i] = 0;
        }
        
        // initialize dynamic fields to NULL
        new_file->name[0] = '\0';
        new_file->type = FILE_NODE; // will be set properly later
        new_file->parent = NULL;

        // initialize union fields to NULL
        new_file->file.content = NULL;
        new_file->file.content_size = 0;
        new_file->file.content_capacity = 0;
        
        file_count++;
    }
    return new_file;
}

/**
 * free a file using free()
 * @param file pointer to the file to free
 */
void file_free(File* file) {
    if (!file) return;
    
    if (file->type == FILE_NODE) {
        // free content
        if (file->file.content) {
            kfree(file->file.content);
            file->file.content = NULL;
        }
    } else if (file->type == FOLDER_NODE) {
        // recursively free all children
        for (int i = 0; i < file->folder.childCount; i++) {
            if (file->folder.children && file->folder.children[i]) {
                file_free(file->folder.children[i]);
            }
        }
        // free the children array
        if (file->folder.children) {
            kfree(file->folder.children);
            file->folder.children = NULL;
        }
    }

    // free the file and decrease file_count because we just deleted the file.
    kfree(file);
    file_count--;
}

/**
 * allocate a new inode number
 */
uint32_t allocate_inode(void) {
    if (!superblock) {
        debugf("[FILE ALLOCATOR] Superblock not initialized!\n");
        return 0;
    }

    for (uint32_t i = 1; i < MAX_DISK_INODES; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        
        if (!(superblock->inode_bitmap[byte_index] & (1 << bit_index))) {
            // mark as allocated
            superblock->inode_bitmap[byte_index] |= (1 << bit_index);
            superblock->free_inode_count--;
            return i;
        }
    }
    debugf("[FILE ALLOCATOR] No free inodes available!\n");
    return 0; // no free inodes
}

/**
 * allocate a new data block
 */
uint32_t allocate_data_block(void) {
    if (!superblock) {
        debugf("[FILE ALLOCATOR] Superblock not initialized!\n");
        return 0;
    }

    for (uint32_t i = 0; i < superblock->data_block_count; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        
        if (!(superblock->block_bitmap[byte_index] & (1 << bit_index))) {
            // mark as allocated
            superblock->block_bitmap[byte_index] |= (1 << bit_index);
            superblock->free_block_count--;
            return DATA_BLOCKS_START_SECTOR + i;
        }
    }
    debugf("[FILE ALLOCATOR] No free data blocks available!\n");
    return 0; // no free blocks
}

/**
 * free an inode
 */
void free_inode(uint32_t inode_num) {
    if (!superblock || inode_num == 0 || inode_num >= MAX_DISK_INODES) {
        debugf("[FILE ALLOCATOR] Invalid inode to free");
        return;
    }
    
    uint32_t byte_index = inode_num / 8;
    uint32_t bit_index = inode_num % 8;
    
    superblock->inode_bitmap[byte_index] &= ~(1 << bit_index);
    superblock->free_inode_count++;
}

/**
 * free a data block
 */
void free_data_block(uint32_t block_num) {
    if (!superblock || block_num < DATA_BLOCKS_START_SECTOR) {
        debugf("[FILE ALLOCATOR] Invalid block to free");
        return;
    }
    
    uint32_t block_index = block_num - DATA_BLOCKS_START_SECTOR;
    if (block_index >= superblock->data_block_count) {
        debugf("[FILE ALLOCATOR] Block out of range");
        return;
    }

    uint32_t byte_index = block_index / 8;
    uint32_t bit_index = block_index % 8;
    
    superblock->block_bitmap[byte_index] &= ~(1 << bit_index);
    superblock->free_block_count++;
}
