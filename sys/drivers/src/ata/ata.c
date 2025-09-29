/*
    MooseOS ATA Driver
    Copyright (c) 2025 Ethan Zhang
    Licensed under the MIT license. See license file for details
*/
/** @todo: this currently only supports QEMU disks. not a high priority. */

#include "ata/ata.h"
#include "libc/lib.h"
#include "print/debug.h"
#include <stdio.h>

// ATA device information
ata_device ata_devices[4];

/**
 * read status register from ATA controller
 * @return status byte
 */
uint8_t ata_read_status(uint16_t base_io) {
    return inb(base_io + ATA_REG_STATUS);
}

/**
 * write command to ATA controller
 * @param cmd command byte
 * @param base_io base I/O port
 */
void ata_write_command(uint16_t base_io, uint8_t cmd) {
    outb(base_io + ATA_REG_COMMAND, cmd);
}

/**
 * simple delay
 */
void simple_delay(void) {
    for (int i = 0; i < 100; i++) {
        volatile uint8_t dummy = inb(0x80); // Unused port
        /** 
         * @note this code assumes 0x80 is unused
         * if you are having trouble with using 0x80, please change this
        */
        (void)dummy;
    }
}

/**
 * read a sector from disk
 * @param drive drive number (0-3)
 * @param lba logical block address
 * @param buffer buffer to store data
 */
int disk_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        debugf("[ATA] Invalid drive\n"); // tell Ethan/user the code messed up
        return -1; // invalid drive
    }
    
    // use primary IDE controller
    uint16_t base_io = 0x1F0;
    
    // wait for controller to be ready
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) { // timeout
        debugf("[ATA] Timeout waiting for controller\n");
        return -2; 
    } 
    
    // select drive 0 (master) with LBA mode
    outb(base_io + 6, 0xE0 | ((lba >> 24) & 0x0F));
    simple_delay();
    
    // set up the command
    outb(base_io + 2, 1);                    // Sector count = 1
    outb(base_io + 3, lba & 0xFF);           // LBA[7:0]
    outb(base_io + 4, (lba >> 8) & 0xFF);    // LBA[15:8]
    outb(base_io + 5, (lba >> 16) & 0xFF);   // LBA[23:16]
    simple_delay();
    
    // send READ SECTORS command (0x20)
    outb(base_io + 7, 0x20);
    
    // wait for command to complete
    timeout = 10000;
    uint8_t status;
    while (timeout--) {
        status = inb(base_io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                debugf("[ATA] Error during read\n");
                return -3; // error occurred
            }
            if (status & ATA_SR_DRQ) {
                break; // data ready
            }
        }
        simple_delay();
    }
    if (timeout <= 0) {
        debugf("[ATA] Timeout waiting for data\n");
        return -4; // timeout waiting for data
    }

    // read 512 bytes as 256 16-bit words
    uint16_t *word_buffer = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        word_buffer[i] = inw(base_io + 0); // Data port
    }
    
    return 0;
}

/**
 * select drive (master or slave)
 */
void ata_select_drive(uint16_t base_io, uint8_t drive) {
    outb(base_io + ATA_REG_HDDEVSEL, 0xA0 | (drive << 4));

    // wait 400ns after drive selection
    for (int i = 0; i < 4; i++) {
        inb(base_io + ATA_REG_ALTSTATUS);
    }
}

/**
 * soft reset ATA controller
 */
void ata_soft_reset(uint16_t ctrl_io) {
    outb(ctrl_io + ATA_REG_CONTROL, 4); // set SRST bit

    // wait 5 microseconds
    for (int i = 0; i < 1000; i++) {
        inb(ctrl_io);
    }

    outb(ctrl_io + ATA_REG_CONTROL, 0); // clear SRST bit
}

/**
 * wait for drive to be ready
 */
int ata_wait_ready(uint16_t base_io) {
    int timeout = 10000;
    uint8_t status;
    
    while (timeout--) {
        status = ata_read_status(base_io);
        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRDY)) {
            return 0; // ready
        }
        if (status & ATA_SR_ERR) {
            debugf("[ATA] Error waiting for drive ready\n");
            return -1; // error
        }
        simple_delay();
    }
    debugf("[ATA] Timeout waiting for drive ready\n");
    return -2; // timeout
}

/**
 * wait for data request ready
 */
int ata_wait_drq(uint16_t base_io) {
    int timeout = 10000;
    uint8_t status;
    
    while (timeout--) {
        status = ata_read_status(base_io);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                debugf("[ATA] Error waiting for DRQ\n");
                return -1; // error
            }
            if (status & ATA_SR_DRQ) {
                return 0; // data ready
            }
        }
        simple_delay();
    }
    debugf("[ATA] Timeout waiting for DRQ\n");
    return -2; // timeout
}

/**
 * identify ATA drive and get device information
 */
int ata_identify(uint8_t drive, uint16_t *buffer) {
    uint16_t base_io = (drive < 2) ? ATA_PRIMARY_IO_BASE : ATA_SECONDARY_IO_BASE;
    uint8_t drive_sel = drive % 2;
    
    // select drive
    ata_select_drive(base_io, drive_sel);
    
    // clear registers
    outb(base_io + ATA_REG_SECCOUNT0, 0);
    outb(base_io + ATA_REG_LBA0, 0);
    outb(base_io + ATA_REG_LBA1, 0);
    outb(base_io + ATA_REG_LBA2, 0);
    
    // send IDENTIFY command
    ata_write_command(base_io, ATA_CMD_IDENTIFY);
    
    // check if drive exists
    uint8_t status = ata_read_status(base_io);
    if (status == 0) {
        debugf("[ATA] Drive does not exist\n");
        return -1;
    }

    // wait for BSY to clear
    while (ata_read_status(base_io) & ATA_SR_BSY);
    
    // check for non-ATA drive
    if (inb(base_io + ATA_REG_LBA1) != 0 || inb(base_io + ATA_REG_LBA2) != 0) {
        debugf("[ATA] Not an ATA drive\n");
        return -2; // not an ATA drive
    }

    // wait for DRQ or ERR
    while (!(ata_read_status(base_io) & (ATA_SR_DRQ | ATA_SR_ERR)));
    
    if (ata_read_status(base_io) & ATA_SR_ERR) {
        debugf("[ATA] Error during IDENTIFY\n");
        return -3; // error occurred
    }

    // read 256 words of identification data
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(base_io + ATA_REG_DATA);
    }
    
    return 0; // success!
}

/**
 * initialize disk
 */
void disk_init(void) {
    for (int i = 0; i < 4; i++) {
        ata_devices[i].exists = 0;
    }
    
    /**
     * Set up primary master 
     * @note Currently only works on QEMU
     */
    ata_devices[0].exists = 1;
    ata_devices[0].drive = 0;
    ata_devices[0].base_io = 0x1F0;
    ata_devices[0].ctrl_io = 0x3F6;
    ata_devices[0].size = 20480; // 10mb
    strcpy(ata_devices[0].model, "QEMU");
}

/**
 * write a single sector to disk
 */
int disk_write_sector(uint8_t drive, uint32_t lba, uint8_t *buffer) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        debugf("[ATA] Invalid drive\n");
        return -1; // Invalid drive
    }
    
    // use primary ATA controller (0x1F0)
    uint16_t base_io = 0x1F0;

    // wait for controller to be ready (BSY clear)
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) { // timeout waiting for ready
        debugf("[ATA] Timeout waiting for controller\n");
        return -2;
    }
    
    // select drive 0 (master) with LBA mode
    outb(base_io + 6, 0xE0 | ((lba >> 24) & 0x0F));
    simple_delay();
    
    // set up the command
    outb(base_io + 2, 1);                    // sector count = 1
    outb(base_io + 3, lba & 0xFF);           // LBA[7:0]
    outb(base_io + 4, (lba >> 8) & 0xFF);    // LBA[15:8]
    outb(base_io + 5, (lba >> 16) & 0xFF);   // LBA[23:16]
    simple_delay();
    
    // send WRITE SECTORS command (0x30)
    outb(base_io + 7, 0x30);

    // wait for command to be ready for data
    timeout = 10000;
    uint8_t status;
    while (timeout--) {
        status = inb(base_io + ATA_REG_STATUS);
        if (!(status & ATA_SR_BSY)) {
            if (status & ATA_SR_ERR) {
                debugf("[ATA] Error during write\n");
                return -3; // error occurred
            }
            if (status & ATA_SR_DRQ) {
                break; // ready for data
            }
        }
        simple_delay();
    }
    if (timeout <= 0) { // timeout waiting for data ready
        debugf("[ATA] Timeout waiting for data ready\n");
        return -4; 
    }
    
    // write 512 bytes as 256 16-bit words
    uint16_t *word_buffer = (uint16_t*)buffer;
    for (int i = 0; i < 256; i++) {
        outw(base_io + 0, word_buffer[i]); // data port
    }
    
    // wait for write to complete (BSY clear)
    timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    if (timeout <= 0) { // timeout waiting for write completion
        debugf("[ATA] Timeout waiting for write completion\n");
        return -5;
    }
    
    // Check for errors
    status = inb(base_io + ATA_REG_STATUS);
    if (status & ATA_SR_ERR) {
        debugf("[ATA] Write error\n");
        return -6; // Write error
    }
    
    // flush cache to ensure data is written to disk
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);

    // wait for flush to complete
    timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }

    return 0; // success
}

/**
 * read multiple sectors from disk
 */
int disk_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!buffer || sector_count == 0) return -1;
    
    for (uint8_t i = 0; i < sector_count; i++) {
        int result = disk_read_sector(drive, lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            debugf("[ATA] Error reading sector\n");
            return result; // return the specific error code
        }
    }
    return 0;
}

/**
 * write multiple sectors to disk
 */
int disk_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer) {
    if (!buffer || sector_count == 0) return -1;
    
    for (uint8_t i = 0; i < sector_count; i++) {
        int result = disk_write_sector(drive, lba + i, buffer + (i * SECTOR_SIZE));
        if (result != 0) {
            debugf("[ATA] Error writing sector\n");
            return result; // return specific error code
        }
    }
    return 0;
}

/**
 * force flush disk cache
 *
 * @todo: is this necessary?
 * @note it may damage your ATA drive. this is for emergency only
 */
int disk_force_flush(uint8_t drive) {
    if (drive >= 4 || !ata_devices[drive].exists) {
        debugf("[ATA] Invalid drive\n");
        return -1;
    }
    
    uint16_t base_io = 0x1F0;
    
    //  standard cache flush
    outb(base_io + ATA_REG_COMMAND, ATA_CMD_CACHE_FLUSH);
    
    int timeout = 10000;
    while ((inb(base_io + ATA_REG_STATUS) & ATA_SR_BSY) && timeout--) {
        simple_delay();
    }
    
    /**
     * multiple read/write cycles to force persistence
     * 
     * @todo: is this necessary?
     */
    uint8_t temp_buffer[512];
    for (int i = 0; i < 3; i++) {
        // read a sector and write it back
        if (disk_read_sector(drive, 0, temp_buffer) == 0) {
            disk_write_sector(drive, 0, temp_buffer);
        }
        simple_delay();
    }

    return 0; // success! except that this code damages your ATA drive :(
}
