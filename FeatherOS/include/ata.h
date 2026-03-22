/* FeatherOS - ATA/PATA Storage Driver
 * Sprint 17: Storage (ATA/PATA)
 */

#ifndef FEATHEROS_ATA_H
#define FEATHEROS_ATA_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * ATA I/O PORTS
 *============================================================================*/

/* Primary channel */
#define ATA_PRIMARY_BASE      0x1F0
#define ATA_PRIMARY_CTRL      0x3F6

/* Secondary channel */
#define ATA_SECONDARY_BASE    0x170
#define ATA_SECONDARY_CTRL    0x376

/* ATA register offsets */
#define ATA_REG_DATA          0x00  /* Data register (16-bit) */
#define ATA_REG_ERROR         0x01  /* Error register (read) */
#define ATA_REG_FEATURES      0x01  /* Features register (write) */
#define ATA_REG_SECCOUNT      0x02  /* Sector count */
#define ATA_REG_LBA_LO        0x03  /* LBA low byte */
#define ATA_REG_LBA_MID       0x04  /* LBA middle byte */
#define ATA_REG_LBA_HI        0x05  /* LBA high byte */
#define ATA_REG_DRIVE         0x06  /* Drive/head register */
#define ATA_REG_STATUS        0x07  /* Status register (read) */
#define ATA_REG_COMMAND       0x07  /* Command register (write) */

/*============================================================================
 * ATA STATUS REGISTER BITS
 *============================================================================*/

#define ATA_STATUS_ERR        0x01  /* Error */
#define ATA_STATUS_DRQ        0x08  /* Data request */
#define ATA_STATUS_SRV        0x10  /* Overlapped mode service request */
#define ATA_STATUS_DRDY       0x40  /* Drive ready */
#define ATA_STATUS_BSY        0x80  /* Busy */

/*============================================================================
 * ATA CONTROL REGISTER BITS
 *============================================================================*/

#define ATA_CTRL_NIEN         0x02  /* Disable interrupts */
#define ATA_CTRL_SRST         0x04  /* Software reset */
#define ATA_CTRL_HOB          0x80  /* High-order byte (for 48-bit LBA) */

/*============================================================================
 * ATA COMMANDS
 *============================================================================*/

#define ATA_CMD_READ_PIO      0x20  /* Read sectors (PIO) */
#define ATA_CMD_READ_PIO_EXT 0x24  /* Read sectors (PIO) extended */
#define ATA_CMD_WRITE_PIO     0x30  /* Write sectors (PIO) */
#define ATA_CMD_WRITE_PIO_EXT 0x34  /* Write sectors (PIO) extended */
#define ATA_CMD_READ_DMA      0xC8  /* Read sectors (DMA) */
#define ATA_CMD_READ_DMA_EXT  0x25  /* Read sectors (DMA) extended */
#define ATA_CMD_WRITE_DMA     0xCA  /* Write sectors (DMA) */
#define ATA_CMD_WRITE_DMA_EXT 0x35  /* Write sectors (DMA) extended */
#define ATA_CMD_IDENTIFY       0xEC  /* Identify drive */
#define ATA_CMD_SET_FEATURES  0xEF  /* Set features */
#define ATA_CMD_FLUSH_CACHE   0xE7  /* Flush cache */
#define ATA_CMD_FLUSH_EXT     0xEA  /* Flush cache extended */
#define ATA_CMD_PACKET         0xA0  /* ATAPI packet command */
#define ATA_CMD_IDENTIFY_PACKET 0xA1  /* Identify ATAPI device */

/*============================================================================
 * ATA IDENTIFY DATA
 *============================================================================*/

#define ATA_IDENT_GENERAL_CONFIG     0
#define ATA_IDENT_CYLINDERS          1
#define ATA_IDENT_HEADS              2
#define ATA_IDENT_SECTORS            3
#define ATA_IDENT_SERIAL             10
#define ATA_IDENT_MODEL              27
#define ATA_IDENT_CAPABILITIES       49
#define ATA_IDENT_FIELD_VALID         53
#define ATA_IDENT_MAX_LBA            60
#define ATA_IDENT_MULTI_SETTING      71
#define ATA_IDENT_MULTI_VALID        (1 << 0)
#define ATA_IDENT_LBA48_SUPPORTED     (1 << 1)
#define ATA_IDENT_LBA48_ENABLED       (1 << 2)

/*============================================================================
 * DEVICE TYPES
 *============================================================================*/

#define ATA_TYPE_NONE       0
#define ATA_TYPE_ATA        1
#define ATA_TYPE_ATAPI       2

/*============================================================================
 * ATA DEVICE
 *============================================================================*/

typedef struct {
    uint16_t type;              /* Device type (ATA_TYPE_*) */
    uint16_t channels;         /* Number of channels */
    uint16_t selected;          /* Selected device on each channel */
    
    /* Drive info */
    uint32_t sectors_28;        /* Total sectors (28-bit LBA) */
    uint64_t sectors_48;        /* Total sectors (48-bit LBA) */
    bool lba48;                 /* 48-bit LBA supported */
    bool dma;                   /* DMA supported */
    bool lba;                   /* LBA supported */
    bool multi;                 /* Multi-word DMA supported */
    
    /* Model string (40 chars) */
    char model[41];
    char serial[21];
    
    /* I/O ports */
    uint16_t io_base;
    uint16_t ctrl_base;
} ata_device_t;

/*============================================================================
 * PARTITION TABLE
 *============================================================================*/

#define MBR_SIGNATURE       0xAA55
#define MAX_PARTITIONS      4

/* Partition entry in MBR */
typedef struct {
    uint8_t status;            /* Boot indicator */
    uint8_t start_head;        /* Start head */
    uint8_t start_sector;      /* Start sector (bits 0-5) */
    uint8_t start_cylinder;    /* Start cylinder (bits 0-7) */
    uint8_t type;              /* Partition type */
    uint8_t end_head;          /* End head */
    uint8_t end_sector;        /* End sector (bits 0-5) */
    uint8_t end_cylinder;      /* End cylinder (bits 0-7) */
    uint32_t start_lba;        /* Starting LBA */
    uint32_t total_sectors;    /* Total sectors in partition */
} __attribute__((packed)) partition_entry_t;

/* MBR structure */
typedef struct {
    uint8_t boot_code[446];
    partition_entry_t partitions[MAX_PARTITIONS];
    uint16_t signature;
} __attribute__((packed)) mbr_t;

/*============================================================================
 * BLOCK DEVICE
 *============================================================================*/

/* Block device operations */
typedef struct block_device block_device_t;

typedef struct {
    int (*read)(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);
    int (*write)(block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer);
    int (*flush)(block_device_t *dev);
    int (*ioctl)(block_device_t *dev, uint32_t cmd, void *arg);
} block_ops_t;

/* Block device structure */
struct block_device {
    const char *name;
    block_ops_t ops;
    
    uint64_t total_sectors;     /* Total number of sectors */
    uint32_t sector_size;       /* Sector size in bytes */
    uint8_t type;               /* Device type */
    
    void *private_data;         /* Driver-specific data */
};

/*============================================================================
 * DISK BUFFER CACHE
 *============================================================================*/

/* Buffer states */
#define BUFFER_DIRTY       0x01
#define BUFFER_LOCKED      0x02
#define BUFFER_VALID       0x04
#define BUFFER_ASYNC       0x08

typedef struct buffer_head {
    struct buffer_head *next;
    struct buffer_head *prev;
    
    uint64_t sector;            /* Sector number */
    uint32_t size;             /* Buffer size */
    uint32_t flags;            /* Buffer flags */
    
    uint8_t *data;             /* Data buffer */
    
    /* Metadata */
    uint64_t read_time;        /* Last read time */
    uint64_t write_time;       /* Last write time */
    int ref_count;             /* Reference count */
    
    /* Async I/O */
    void (*callback)(void *arg);
    void *callback_arg;
} buffer_head_t;

/*============================================================================
 * I/O REQUEST QUEUE
 *============================================================================*/

/* Request flags */
#define REQ_READ            0x01
#define REQ_WRITE           0x02
#define REQ_SYNC            0x04
#define REQ_FLUSH           0x08

/* Request structure */
typedef struct bio {
    struct bio *next;
    struct bio *prev;
    
    block_device_t *dev;       /* Target device */
    uint64_t sector;          /* Starting sector */
    uint32_t count;           /* Sector count */
    uint32_t flags;           /* Request flags */
    
    void *buffer;             /* Data buffer */
    int error;                /* Error code */
    
    /* Completion */
    void (*callback)(struct bio *bio);
    void *callback_arg;
} bio_t;

/* Request queue */
typedef struct request_queue {
    bio_t *head;
    bio_t *tail;
    uint32_t pending;
    uint32_t max_pending;
} request_queue_t;

/*============================================================================
 * ATA INITIALIZATION
 *============================================================================*/

/* Initialize ATA driver */
int ata_init(void);

/* Detect and initialize ATA devices */
int ata_detect_devices(void);

/*============================================================================
 * ATA OPERATIONS
 *============================================================================*/

/* Read sectors (PIO mode) */
int ata_read(ata_device_t *dev, uint32_t lba, uint8_t count, uint16_t *buffer);

/* Write sectors (PIO mode) */
int ata_write(ata_device_t *dev, uint32_t lba, uint8_t count, uint16_t *buffer);

/* Read sectors (DMA mode) */
int ata_read_dma(ata_device_t *dev, uint32_t lba, uint16_t count, uint32_t *buffer);

/* Write sectors (DMA mode) */
int ata_write_dma(ata_device_t *dev, uint32_t lba, uint16_t count, uint32_t *buffer);

/* Flush cache */
int ata_flush_cache(ata_device_t *dev);

/* Identify drive */
int ata_identify(ata_device_t *dev, uint16_t *buffer);

/*============================================================================
 * BLOCK DEVICE OPERATIONS
 *============================================================================*/

/* Register a block device */
int register_block_device(block_device_t *dev);

/* Unregister a block device */
int unregister_block_device(block_device_t *dev);

/* Get block device by name */
block_device_t *get_block_device(const char *name);

/* Read from block device */
int block_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer);

/* Write to block device */
int block_write(block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer);

/* Flush block device */
int block_flush(block_device_t *dev);

/*============================================================================
 * PARTITION TABLE
 *============================================================================*/

/* Read MBR from device */
int read_mbr(block_device_t *dev, mbr_t *mbr);

/* Parse partition table */
int parse_partitions(block_device_t *dev, partition_entry_t *partitions);

/* Print partition table */
void print_partitions(partition_entry_t *partitions);

/*============================================================================
 * BUFFER CACHE
 *============================================================================*/

/* Initialize buffer cache */
void buffer_cache_init(void);

/* Get buffer for sector */
buffer_head_t *bread(block_device_t *dev, uint64_t sector, uint32_t size);

/* Get buffer and mark for write */
buffer_head_t *getblk(block_device_t *dev, uint64_t sector, uint32_t size);

/* Mark buffer dirty and release */
void brelse(buffer_head_t *bh);

/* Write buffer synchronously */
int bwrite(buffer_head_t *bh);

/* Submit buffer for async I/O */
void submit_bio(buffer_head_t *bh, int write, 
                void (*callback)(void *), void *arg);

/* Flush all dirty buffers for device */
void sync_dev(block_device_t *dev);

/*============================================================================
 * REQUEST QUEUE
 *============================================================================*/

/* Initialize request queue */
void init_request_queue(request_queue_t *q, uint32_t max_pending);

/* Add request to queue */
void add_request(request_queue_t *q, bio_t *bio);

/* Get next request from queue */
bio_t *get_request(request_queue_t *q);

/* Process request queue */
void process_requests(request_queue_t *q);

/*============================================================================
 * STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t reads;
    uint64_t writes;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t dma_reads;
    uint64_t dma_writes;
} ata_stats_t;

ata_stats_t *ata_get_stats(void);
void ata_print_stats(void);

#endif /* FEATHEROS_ATA_H */
