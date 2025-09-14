/*
    Copyright (c) 2025 Ethan Zhang and Contributors.
*/

/*
    ==================== OS THEORY ====================
    If you haven't read other OS theory files, basically MooseOS is an educational OS, so comments at the top of each 
    file will explain the relevant OS theory. This is so that users can learn about OS concepts while reading the code, 
    and maybe even make their own OS some day. 
    Usually, there are external websites that describe OS Theory excellently. They will be quoted, and a link
    will be provided.
    
    ATA/IDE INTERFACE:
    ATA (Advanced Technology Attachment) is a standard way for CPUs to talk to disks.
    IDE (Integrated Drive Electronics) is the older name.
    
    ATA COMMAND PROTOCOL:
    Communication happens through I/O ports:
    - COMMAND PORT: Send commands (READ, WRITE, IDENTIFY)
    - STATUS PORT: Check if drive is busy/ready/error
    - DATA PORT: Transfer actual data (512 bytes per sector)
    - FEATURES PORT: Specify command parameters

    LBA? WHAT'S AN LBA?
    LBA stands for Logical Block Addressing.  It is a way of determining the position of a block.
    Instead of specifying cylinder/head/sector, just use a single number:
    - Sector 0, Sector 1, Sector 2, ... up to millions
    - Each sector is typically 512 bytes
    - Much simpler than old CHS (Cylinder/Head/Sector) addressing

    Source: https://wiki.osdev.org/ATA_PIO_Mode
            https://wiki.osdev.org/ATA_Command_Matrix (in case you need to write an ATA driver)

*/

/** @todo: This currently only supports QEMU disks. Not a high priority. */

#include "include/disk.h"
#include "../lib/lib.h"

// ATA device information
ata_device_t ata_devices[4];

/**
 * Read status register from ATA controller
 */
uint8_t ata_read_status(uint16_t base_io) {
    return inb(base_io + ATA_REG_STATUS);
}

/**
 * Write command to ATA controller
 */
void ata_write_command(uint16_t base_io, uint8_t cmd) {
    outb(base_io + ATA_REG_COMMAND, cmd);
}

/**
 * Delay
 */
void simple_delay(void) {
    for (int i = 0; i < 100; i++) {
        volatile uint8_t dummy = inb(0x80); // Unused port
        (void)dummy;
    }
}

/**
 * Read a single sector from disk
 */
int disk_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        return -1; // Invalid drive
    }
    
    // Use primary IDE controller
    uint16_t base_io = 0x1F0;
    
    // Wait for controller to be ready (BSY clear)
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) return -2; // Timeout waiting for ready
    
    // Select drive 0 (master) with LBA mode
    outb(base_io + 6, 0xE0 | ((lba >> 24) & 0x0F));
    simple_delay();
    
    // Set up the command
    outb(base_io + 2, 1);                    // Sector count = 1
    outb(base_io + 3, lba & 0xFF);           // LBA[7:0]
    outb(base_io + 4, (lba >> 8) & 0xFF);    // LBA[15:8]
    outb(base_io + 5, (lba >> 16) & 0xFF);   // LBA[23:16]
    simple_delay();
    
    // Send READ SECTORS command (0x20)
    outb(base_io + 7, 0x20);
    
    // Wait for command to complete (BSY clear and either DRQ or ERR)
    timeout = 10000;
    uint8_t status;
    while (timeout--) {
        status = inb(base_io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                return -3; // Error occurred
            }
            if (status & ATA_SR_DRQ) {
                break; // Data ready
            }
        }
        simple_delay();
    }
    if (timeout <= 0) return -4; // Timeout waiting for data
    
    // Read 512 bytes as 256 16-bit words
    uint16_t *word_buffer = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        word_buffer[i] = inw(base_io + 0); // Data port
    }
    
    return 0;
}

/**
 * Select drive (master or slave)
 */
void ata_select_drive(uint16_t base_io, uint8_t drive) {
    outb(base_io + ATA_REG_HDDEVSEL, 0xA0 | (drive << 4));
    
    // Wait 400ns after drive selection
    for (int i = 0; i < 4; i++) {
        inb(base_io + ATA_REG_ALTSTATUS);
    }
}

/**
 * Perform soft reset on ATA controller
 */
void ata_soft_reset(uint16_t ctrl_io) {
    outb(ctrl_io + ATA_REG_CONTROL, 4); // Set SRST bit
    
    // Wait 5 microseconds
    for (int i = 0; i < 1000; i++) {
        inb(ctrl_io);
    }
    
    outb(ctrl_io + ATA_REG_CONTROL, 0); // Clear SRST bit
}

/**
 * Wait for drive to be ready (BSY clear, DRDY set)
 */
int ata_wait_ready(uint16_t base_io) {
    int timeout = 10000;
    uint8_t status;
    
    while (timeout--) {
        status = ata_read_status(base_io);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            return 0; // Ready
        }
        if (status & ATA_SR_ERR) {
            return -1; // Error
        }
        simple_delay();
    }
    return -2; // Timeout
}

/**
 * Wait for data request (DRQ set, BSY clear)
 */
int ata_wait_drq(uint16_t base_io) {
    int timeout = 10000;
    uint8_t status;
    
    while (timeout--) {
        status = ata_read_status(base_io);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                return -1; // Error
            }
            if (status & ATA_SR_DRQ) {
                return 0; // Data ready
            }
        }
        simple_delay();
    }
    return -2; // Timeout
}

/**
 * Identify ATA drive and get device information
 */
int ata_identify(uint8_t drive, uint16_t *buffer) {
    uint16_t base_io = (drive < 2) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;
    uint8_t drive_sel = drive % 2;
    
    // Select drive
    ata_select_drive(base_io, drive_sel);
    
    // Clear registers
    outb(base_io + ATA_REG_SECCOUNT0, 0);
    outb(base_io + ATA_REG_LBA0, 0);
    outb(base_io + ATA_REG_LBA1, 0);
    outb(base_io + ATA_REG_LBA2, 0);
    
    // Send IDENTIFY command
    ata_write_command(base_io, ATA_CMD_IDENTIFY);
    
    // Check if drive exists
    uint8_t status = ata_read_status(base_io);
    if (status == 0) {
        return -1; // Drive does not exist
    }
    
    // Wait for BSY to clear
    while (ata_read_status(base_io) & ATA_SR_BSY);
    
    // Check for non-ATA drive
    if (inb(base_io + ATA_REG_LBA1) != 0 || inb(base_io + ATA_REG_LBA2) != 0) {
        return -2; // Not an ATA drive
    }
    
    // Wait for DRQ or ERR
    while (!(ata_read_status(base_io) & (ATA_SR_DRQ | ATA_SR_ERR)));
    
    if (ata_read_status(base_io) & ATA_SR_ERR) {
        return -3; // Error occurred
    }
    
    // Read 256 words of identification data
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base_io + ATA_REG_DATA);
    }
    
    return 0; // Success
}

/**
 * Initialize disk
 */
void disk_init(void) {
    for (int i = 0; i < 4; i++) {
        ata_devices[i].exists = 0;
    }
    
    // Set up primary master 
    // Currently only works on QEMU
    ata_devices[0].exists = 1;
    ata_devices[0].drive = 0;
    ata_devices[0].base_io = 0x1F0;
    ata_devices[0].ctrl_io = 0x3F6;
    ata_devices[0].size = 20480; // 10mb
    copyStr(ata_devices[0].model, "QEMU");
}

/**
 * Write a single sector to disk
 */
int disk_write_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        return -1; // Invalid drive
    }
    
    // Use primary IDE controller (0x1F0)
    uint16_t base_io = 0x1F0;
    
    // Wait for controller to be ready (BSY clear)
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) return -2; // Timeout waiting for ready
    
    // Select drive 0 (master) with LBA mode
    outb(base_io + 6, 0xE0 | ((lba >> 24) & 0x0F));
    simple_delay();
    
    // Set up the command
    outb(base_io + 2, 1);                    // Sector count = 1
    outb(base_io + 3, lba & 0xFF);           // LBA[7:0]
    outb(base_io + 4, (lba >> 8) & 0xFF);    // LBA[15:8]
    outb(base_io + 5, (lba >> 16) & 0xFF);   // LBA[23:16]
    simple_delay();
    
    // Send WRITE SECTORS command (0x30)
    outb(base_io + 7, 0x30);
    
    // Wait for command to be ready for data (BSY clear and DRQ set)
    timeout = 10000;
    uint8_t status;
    while (timeout--) {
        status = inb(base_io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                return -3; // Error occurred
            }
            if (status & ATA_SR_DRQ) {
                break; // Ready for data
            }
        }
        simple_delay();
    }
    if (timeout <= 0) return -4; // Timeout waiting for data ready
    
    // Write 512 bytes as 256 16-bit words
    uint16_t *word_buffer = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(base_io + 0, word_buffer[i]); // Data port
    }
    
    // Wait for write to complete (BSY clear)
    timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) return -5; // Timeout waiting for completion
    
    // Check for errors
    status = inb(base_io + ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
        return -6; // Write error
    }
    
    // Flush cache to ensure data is written to disk
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    // Wait for flush to complete
    timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    
    return 0;
}

/**
 * Read multiple sectors from disk
 */
int disk_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!buffer || sector_count == 0) return -1;
    
    for (uint8_t i = 0; i < sector_count; i++) {
        int result = disk_read_sector(drive, lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            return result; // Return the specific error code
        }
    }
    return 0;
}

/**
 * Write multiple sectors to disk
 */
int disk_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!buffer || sector_count == 0) return -1;
    
    for (uint8_t i = 0; i < sector_count; i++) {
        int result = disk_write_sector(drive, lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            return result; // Return the specific error code
        }
    }
    return 0;
}

/**
 * Force flush disk cache using multiple methods
 *
 * @todo: Is this necessary?
 */
int disk_force_flush(uint8_t drive) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        return -1;
    }
    
    uint16_t base_io = 0x1F0;
    
    //  Standard cache flush
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    
    /**
     * Multiple read/write cycles to force persistence
     * 
     * @todo: Is this necessary?
     */
    uint8_t temp_buffer[512];
    for (int i = 0; i < 3; i++) {
        // Read a sector and write it back
        if (disk_read_sector(drive, 0, temp_buffer) == 0) {
            disk_write_sector(drive, 0, temp_buffer);
        }
        simple_delay();
    }
    
    return 0;
}
