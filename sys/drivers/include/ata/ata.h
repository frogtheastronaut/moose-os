/*
    MooseOS ATA Driver
    Copyright (c) 2025 Ethan Zhang
    All rights reserved
*/

#ifndef DISK_H
#define DISK_H

#include "io/io.h"
#include <stdint.h>

// ATA addresses
#define ATA_PRIMARY_IO_BASE     0x1F0
#define ATA_SECONDARY_IO_BASE   0x170
#define ATA_PRIMARY_CTRL_BASE   0x3F6
#define ATA_SECONDARY_CTRL_BASE 0x376

// ATA registers
#define ATA_REG_DATA       0x00
#define ATA_REG_ERROR      0x01
#define ATA_REG_FEATURES   0x01
#define ATA_REG_SECCOUNT0  0x02
#define ATA_REG_LBA0       0x03
#define ATA_REG_LBA1       0x04
#define ATA_REG_LBA2       0x05
#define ATA_REG_HDDEVSEL   0x06
#define ATA_REG_COMMAND    0x07
#define ATA_REG_STATUS     0x07
#define ATA_REG_SECCOUNT1  0x08
#define ATA_REG_LBA3       0x09
#define ATA_REG_LBA4       0x0A
#define ATA_REG_LBA5       0x0B
#define ATA_REG_CONTROL    0x0C
#define ATA_REG_ALTSTATUS  0x0C
#define ATA_REG_DEVADDRESS 0x0D

// ATA commands
#define ATA_CMD_READ_PIO          0x20
#define ATA_CMD_READ_PIO_EXT      0x24
#define ATA_CMD_READ_DMA          0xC8
#define ATA_CMD_READ_DMA_EXT      0x25
#define ATA_CMD_WRITE_PIO         0x30
#define ATA_CMD_WRITE_PIO_EXT     0x34
#define ATA_CMD_WRITE_DMA         0xCA
#define ATA_CMD_WRITE_DMA_EXT     0x35
#define ATA_CMD_CACHE_FLUSH       0xE7
#define ATA_CMD_CACHE_FLUSH_EXT   0xEA
#define ATA_CMD_PACKET            0xA0
#define ATA_CMD_IDENTIFY_PACKET   0xA1
#define ATA_CMD_IDENTIFY          0xEC

// ATA Status Register 
#define ATA_SR_BSY     0x80    // busy
#define ATA_SR_DRDY    0x40    // drive ready
#define ATA_SR_DF      0x20    // drive write fault
#define ATA_SR_DSC     0x10    // drive seek complete
#define ATA_SR_DRQ     0x08    // data request ready
#define ATA_SR_CORR    0x04    // corrected data
#define ATA_SR_IDX     0x02    // index
#define ATA_SR_ERR     0x01    // error!

// ATA error registers
#define ATA_ER_BBK      0x80    // bad sector
#define ATA_ER_UNC      0x40    // uncorrectable data
#define ATA_ER_MC       0x20    // media changed
#define ATA_ER_IDNF     0x10    // ID mark not found
#define ATA_ER_MCR      0x08    // media change request
#define ATA_ER_ABRT     0x04    // command aborted
#define ATA_ER_TK0NF    0x02    // track 0 not found
#define ATA_ER_AMNF     0x01    // no address mark

// drive selection
#define ATA_MASTER     0x00
#define ATA_SLAVE      0x01

// sector size
#define SECTOR_SIZE    512

// ATA device structure
typedef struct {
    uint16_t base_io;
    uint16_t ctrl_io;
    uint8_t drive;      // 0 for master, 1 for slave
    uint8_t exists;     // 1 if drive exists, 0 if not
    uint32_t size;      // Size in sectors
    char model[41];     // drive identification string
} ata_device;

// function prototypes
void disk_init(void);
int disk_read_sector(uint8_t drive, uint32_t lba, uint8_t *buffer);
int disk_write_sector(uint8_t drive, uint32_t lba, uint8_t *buffer);
int disk_read_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer);
int disk_write_sectors(uint8_t drive, uint32_t lba, uint8_t sector_count, uint8_t *buffer);
int disk_force_flush(uint8_t drive);
uint8_t ata_read_status(uint16_t base_io);
void ata_write_command(uint16_t base_io, uint8_t cmd);
// return codes: 0=success, -1=error, -2=timeout
int ata_wait_ready(uint16_t base_io);
int ata_wait_drq(uint16_t base_io);
void ata_select_drive(uint16_t base_io, uint8_t drive);
int ata_identify(uint8_t drive, uint16_t *buffer);
void ata_soft_reset(uint16_t ctrl_io);

// external variables
extern ata_device ata_devices[4]; // primary master/slave, secondary master/slave

#endif // DISK_H
