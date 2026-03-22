/* FeatherOS - ATA/PATA Storage Driver Implementation
 * Sprint 17: Storage (ATA/PATA)
 */

#include <ata.h>
#include <kernel.h>
#include <datastructures.h>
#include <sync.h>
#include <timer.h>

/*============================================================================
 * ATA DEVICES
 *============================================================================*/

static ata_device_t ata_devices[4] = {0};
static int ata_device_count = 0;
static block_device_t block_devices[8] = {0};
static int block_device_count = 0;

/*============================================================================
 * ATA STATISTICS
 *============================================================================*/

static ata_stats_t ata_stats = {0};

/*============================================================================
 * ATA HELPER FUNCTIONS
 *============================================================================*/

/* Wait for ATA device to be ready */
static int ata_wait_ready(uint16_t base) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(base + ATA_REG_STATUS);
        if (status & ATA_STATUS_ERR) return -1;
        if (status & ATA_STATUS_DRDY) return 0;
        if (!(status & ATA_STATUS_BSY)) break;
    }
    return -1;
}

/* Wait for device to not be busy */
static int ata_wait_not_busy(uint16_t base) {
    for (int i = 0; i < 100000; i++) {
        uint8_t status = inb(base + ATA_REG_STATUS);
        if (!(status & ATA_STATUS_BSY)) return 0;
    }
    return -1;
}

/* Select drive (0 or 1) */
static void ata_select_drive(uint16_t base, int drive) {
    outb(base + ATA_REG_DRIVE, 0xA0 | (drive << 4));
}

/*============================================================================
 * ATA DETECTION
 *============================================================================*/

static int ata_identify_device(ata_device_t *dev, uint16_t *buffer) {
    /* Wait for device to be ready */
    if (ata_wait_ready(dev->io_base) != 0) {
        return -1;
    }
    
    /* Select drive */
    ata_select_drive(dev->io_base, 0);
    ata_wait_ready(dev->io_base);
    
    /* Send IDENTIFY command */
    outb(dev->io_base + ATA_REG_COMMAND, ATA_CMD_IDENTIFY);
    ata_wait_ready(dev->io_base);
    
    /* Check if device exists */
    uint8_t status = inb(dev->io_base + ATA_REG_STATUS);
    if (status == 0) return -1;  /* No device */
    
    /* Read identify data */
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(dev->io_base + ATA_REG_DATA);
    }
    
    /* Check if valid */
    if (buffer[0] == 0) return -1;  /* Not valid */
    
    /* Parse identify data */
    dev->lba = (buffer[49] >> 9) & 1;
    dev->dma = (buffer[49] >> 8) & 1;
    dev->multi = (buffer[53] >> 0) & 1;
    dev->lba48 = (buffer[83] >> 10) & 1;
    
    /* Get sectors */
    if (dev->lba48) {
        dev->sectors_48 = ((uint64_t)buffer[100] << 0) |
                         ((uint64_t)buffer[101] << 16) |
                         ((uint64_t)buffer[102] << 32) |
                         ((uint64_t)buffer[103] << 48);
        dev->sectors_28 = 0xFFFFFFFF;
    } else if (dev->lba) {
        dev->sectors_28 = ((uint32_t)buffer[60] << 0) |
                          ((uint32_t)buffer[61] << 16);
        dev->sectors_48 = dev->sectors_28;
    } else {
        dev->sectors_28 = ((uint32_t)buffer[1] << 0) |
                          ((uint32_t)buffer[3] << 16);
        dev->sectors_48 = dev->sectors_28;
    }
    
    /* Get model string */
    for (int i = 0; i < 20; i++) {
        uint16_t w = buffer[27 + i];
        dev->model[i * 2] = w >> 8;
        dev->model[i * 2 + 1] = w & 0xFF;
    }
    dev->model[40] = '\0';
    
    /* Get serial number */
    for (int i = 0; i < 10; i++) {
        uint16_t w = buffer[10 + i];
        dev->serial[i * 2] = w >> 8;
        dev->serial[i * 2 + 1] = w & 0xFF;
    }
    dev->serial[20] = '\0';
    
    return 0;
}

int ata_detect_devices(void) {
    uint16_t buffer[256];
    ata_device_count = 0;
    
    console_print("Detecting ATA devices...\n");
    
    /* Primary channel, master */
    ata_devices[0].io_base = ATA_PRIMARY_BASE;
    ata_devices[0].ctrl_base = ATA_PRIMARY_CTRL;
    
    if (ata_identify_device(&ata_devices[0], buffer) == 0) {
        ata_devices[0].type = ATA_TYPE_ATA;
        console_print("  Primary master: %s (%lu sectors)\n",
                     ata_devices[0].model, ata_devices[0].sectors_48);
        ata_device_count++;
    }
    
    /* Primary channel, slave */
    ata_devices[1].io_base = ATA_PRIMARY_BASE;
    ata_devices[1].ctrl_base = ATA_PRIMARY_CTRL;
    ata_select_drive(ATA_PRIMARY_BASE, 1);
    delay_ms(2);
    
    if (ata_identify_device(&ata_devices[1], buffer) == 0) {
        ata_devices[1].type = ATA_TYPE_ATA;
        console_print("  Primary slave: %s (%lu sectors)\n",
                     ata_devices[1].model, ata_devices[1].sectors_48);
        ata_device_count++;
    }
    
    /* Secondary channel, master */
    ata_devices[2].io_base = ATA_SECONDARY_BASE;
    ata_devices[2].ctrl_base = ATA_SECONDARY_CTRL;
    
    if (ata_identify_device(&ata_devices[2], buffer) == 0) {
        ata_devices[2].type = ATA_TYPE_ATA;
        console_print("  Secondary master: %s (%lu sectors)\n",
                     ata_devices[2].model, ata_devices[2].sectors_48);
        ata_device_count++;
    }
    
    /* Secondary channel, slave */
    ata_devices[3].io_base = ATA_SECONDARY_BASE;
    ata_devices[3].ctrl_base = ATA_SECONDARY_CTRL;
    ata_select_drive(ATA_SECONDARY_BASE, 1);
    delay_ms(2);
    
    if (ata_identify_device(&ata_devices[3], buffer) == 0) {
        ata_devices[3].type = ATA_TYPE_ATA;
        console_print("  Secondary slave: %s (%lu sectors)\n",
                     ata_devices[3].model, ata_devices[3].sectors_48);
        ata_device_count++;
    }
    
    console_print("ATA detection complete: %d devices found\n", ata_device_count);
    
    return ata_device_count;
}

/*============================================================================
 * ATA READ/WRITE (PIO MODE)
 *============================================================================*/

int ata_read(ata_device_t *dev, uint32_t lba, uint8_t count, uint16_t *buffer) {
    if (!dev || !buffer || count == 0) return -1;
    if (!dev->lba) return -1;  /* LBA not supported */
    
    uint16_t base = dev->io_base;
    
    /* Wait for device to be ready */
    if (ata_wait_not_busy(base) != 0) return -1;
    
    /* Select drive */
    outb(base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Wait again */
    ata_wait_ready(base);
    
    /* Set sector count and LBA */
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    /* Send read command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_READ_PIO);
    
    /* Read data */
    for (int i = 0; i < count; i++) {
        /* Wait for DRQ */
        for (int j = 0; j < 10000; j++) {
            uint8_t status = inb(base + ATA_REG_STATUS);
            if (status & ATA_STATUS_ERR) return -1;
            if (status & ATA_STATUS_DRQ) break;
        }
        
        /* Read 256 words (one sector) */
        insw(base + ATA_REG_DATA, buffer + (i * 256), 256);
    }
    
    ata_stats.reads++;
    ata_stats.read_bytes += count * 512;
    
    return 0;
}

int ata_write(ata_device_t *dev, uint32_t lba, uint8_t count, uint16_t *buffer) {
    if (!dev || !buffer || count == 0) return -1;
    if (!dev->lba) return -1;
    
    uint16_t base = dev->io_base;
    
    /* Wait for device to be ready */
    if (ata_wait_not_busy(base) != 0) return -1;
    
    /* Select drive */
    outb(base + ATA_REG_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    
    /* Wait again */
    ata_wait_ready(base);
    
    /* Set sector count and LBA */
    outb(base + ATA_REG_SECCOUNT, count);
    outb(base + ATA_REG_LBA_LO, lba & 0xFF);
    outb(base + ATA_REG_LBA_MID, (lba >> 8) & 0xFF);
    outb(base + ATA_REG_LBA_HI, (lba >> 16) & 0xFF);
    
    /* Send write command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_WRITE_PIO);
    
    /* Write data */
    for (int i = 0; i < count; i++) {
        /* Wait for DRQ */
        for (int j = 0; j < 10000; j++) {
            uint8_t status = inb(base + ATA_REG_STATUS);
            if (status & ATA_STATUS_ERR) return -1;
            if (status & ATA_STATUS_DRQ) break;
        }
        
        /* Write 256 words (one sector) */
        outsw(base + ATA_REG_DATA, buffer + (i * 256), 256);
    }
    
    ata_stats.writes++;
    ata_stats.write_bytes += count * 512;
    
    return 0;
}

int ata_flush_cache(ata_device_t *dev) {
    if (!dev) return -1;
    
    uint16_t base = dev->io_base;
    
    /* Wait for device to be ready */
    if (ata_wait_not_busy(base) != 0) return -1;
    
    /* Send flush cache command */
    outb(base + ATA_REG_COMMAND, ATA_CMD_FLUSH_CACHE);
    
    /* Wait for completion */
    ata_wait_not_busy(base);
    
    return 0;
}

int ata_identify(ata_device_t *dev, uint16_t *buffer) {
    if (!dev || !buffer) return -1;
    return ata_identify_device(dev, buffer);
}

/*============================================================================
 * BLOCK DEVICE OPERATIONS
 *============================================================================*/

static int ata_block_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    ata_device_t *ata = (ata_device_t *)dev->private_data;
    return ata_read(ata, (uint32_t)lba, (uint8_t)count, (uint16_t *)buffer);
}

static int ata_block_write(block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    ata_device_t *ata = (ata_device_t *)dev->private_data;
    return ata_write(ata, (uint32_t)lba, (uint8_t)count, (uint16_t *)buffer);
}

static int ata_block_flush(block_device_t *dev) {
    ata_device_t *ata = (ata_device_t *)dev->private_data;
    return ata_flush_cache(ata);
}

static block_ops_t ata_block_ops = {
    .read = ata_block_read,
    .write = ata_block_write,
    .flush = ata_block_flush,
    .ioctl = NULL
};

int register_block_device(block_device_t *dev) {
    if (block_device_count >= 8) return -1;
    
    block_devices[block_device_count] = *dev;
    block_device_count++;
    
    return 0;
}

int unregister_block_device(block_device_t *dev) {
    (void)dev;
    return 0;
}

block_device_t *get_block_device(const char *name) {
    for (int i = 0; i < block_device_count; i++) {
        if (strcmp(block_devices[i].name, name) == 0) {
            return &block_devices[i];
        }
    }
    return NULL;
}

int block_read(block_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    if (!dev || !dev->ops.read) return -1;
    return dev->ops.read(dev, lba, count, buffer);
}

int block_write(block_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    if (!dev || !dev->ops.write) return -1;
    return dev->ops.write(dev, lba, count, (void *)buffer);
}

int block_flush(block_device_t *dev) {
    if (!dev || !dev->ops.flush) return -1;
    return dev->ops.flush(dev);
}

/*============================================================================
 * PARTITION TABLE
 *============================================================================*/

int read_mbr(block_device_t *dev, mbr_t *mbr) {
    if (!dev || !mbr) return -1;
    
    /* Read first sector */
    if (block_read(dev, 0, 1, mbr) != 0) {
        return -1;
    }
    
    /* Check signature */
    if (mbr->signature != MBR_SIGNATURE) {
        return -1;
    }
    
    return 0;
}

int parse_partitions(block_device_t *dev, partition_entry_t *partitions) {
    if (!dev || !partitions) return -1;
    
    mbr_t mbr;
    if (read_mbr(dev, &mbr) != 0) {
        return -1;
    }
    
    /* Copy partition entries */
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        partitions[i] = mbr.partitions[i];
    }
    
    return 0;
}

void print_partitions(partition_entry_t *partitions) {
    if (!partitions) return;
    
    console_print("Partition table:\n");
    console_print("  #  Type    Start     Length\n");
    console_print("  --------------------------------\n");
    
    for (int i = 0; i < MAX_PARTITIONS; i++) {
        if (partitions[i].total_sectors > 0) {
            console_print("  %d  0x%02X   %lu       %lu\n",
                        i + 1,
                        partitions[i].type,
                        (uint64_t)partitions[i].start_lba,
                        (uint64_t)partitions[i].total_sectors);
        }
    }
}

/*============================================================================
 * BUFFER CACHE
 *============================================================================*/

#define BUFFER_CACHE_SIZE    32
static buffer_head_t buffer_cache[BUFFER_CACHE_SIZE];
static buffer_head_t *buffer_hashtable[64];
static spinlock_t buffer_lock = SPINLOCK_UNLOCKED;

void buffer_cache_init(void) {
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        buffer_cache[i].data = NULL;
        buffer_cache[i].flags = 0;
        buffer_cache[i].ref_count = 0;
    }
    
    for (int i = 0; i < 64; i++) {
        buffer_hashtable[i] = NULL;
    }
    
    console_print("Buffer cache initialized (%d buffers)\n", BUFFER_CACHE_SIZE);
}

static uint32_t hash_sector(uint64_t sector) {
    return (uint32_t)(sector & 0x3F);
}

buffer_head_t *bread(block_device_t *dev, uint64_t sector, uint32_t size) {
    if (!dev || !dev->private_data) return NULL;
    
    spin_lock(&buffer_lock);
    
    /* Check if buffer is in cache */
    uint32_t hash = hash_sector(sector);
    buffer_head_t *bh = buffer_hashtable[hash];
    
    while (bh) {
        if (bh->sector == sector && (bh->flags & BUFFER_VALID)) {
            bh->ref_count++;
            spin_unlock(&buffer_lock);
            return bh;
        }
        bh = bh->next;
    }
    
    /* Find a free buffer */
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (buffer_cache[i].ref_count == 0) {
            bh = &buffer_cache[i];
            
            /* Write back if dirty */
            if (bh->flags & BUFFER_DIRTY) {
                block_write(dev, bh->sector, 1, bh->data);
            }
            
            /* Setup buffer */
            bh->sector = sector;
            bh->size = size;
            bh->flags = 0;
            bh->ref_count = 1;
            
            /* Allocate data buffer */
            if (!bh->data) {
                bh->data = (uint8_t *)kmalloc(512);
            }
            
            /* Read data from disk */
            block_read(dev, sector, 1, bh->data);
            bh->flags |= BUFFER_VALID;
            
            /* Add to hash table */
            bh->next = buffer_hashtable[hash];
            buffer_hashtable[hash] = bh;
            
            ata_stats.cache_misses++;
            spin_unlock(&buffer_lock);
            return bh;
        }
    }
    
    spin_unlock(&buffer_lock);
    return NULL;
}

buffer_head_t *getblk(block_device_t *dev, uint64_t sector, uint32_t size) {
    if (!dev || !dev->private_data) return NULL;
    
    spin_lock(&buffer_lock);
    
    /* Check if buffer is in cache */
    uint32_t hash = hash_sector(sector);
    buffer_head_t *bh = buffer_hashtable[hash];
    
    while (bh) {
        if (bh->sector == sector) {
            bh->ref_count++;
            spin_unlock(&buffer_lock);
            return bh;
        }
        bh = bh->next;
    }
    
    /* Find a free buffer */
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (buffer_cache[i].ref_count == 0) {
            bh = &buffer_cache[i];
            
            /* Write back if dirty */
            if (bh->flags & BUFFER_DIRTY) {
                block_write(dev, bh->sector, 1, bh->data);
            }
            
            /* Setup buffer */
            bh->sector = sector;
            bh->size = size;
            bh->flags = BUFFER_DIRTY;
            bh->ref_count = 1;
            
            /* Allocate data buffer */
            if (!bh->data) {
                bh->data = (uint8_t *)kmalloc(512);
            }
            
            /* Add to hash table */
            bh->next = buffer_hashtable[hash];
            buffer_hashtable[hash] = bh;
            
            spin_unlock(&buffer_lock);
            return bh;
        }
    }
    
    spin_unlock(&buffer_lock);
    return NULL;
}

void brelse(buffer_head_t *bh) {
    if (!bh) return;
    
    spin_lock(&buffer_lock);
    bh->ref_count--;
    spin_unlock(&buffer_lock);
}

int bwrite(buffer_head_t *bh) {
    if (!bh || !bh->data) return -1;
    
    /* Would write to disk here */
    bh->flags &= ~BUFFER_DIRTY;
    
    return 0;
}

void submit_bio(buffer_head_t *bh, int write, void (*callback)(void *), void *arg) {
    if (!bh) return;
    
    bh->callback = callback;
    bh->callback_arg = arg;
    
    /* Would submit async I/O here */
    if (write) {
        bwrite(bh);
    }
    
    if (callback) {
        callback(arg);
    }
}

void sync_dev(block_device_t *dev) {
    if (!dev) return;
    
    spin_lock(&buffer_lock);
    
    for (int i = 0; i < BUFFER_CACHE_SIZE; i++) {
        if (buffer_cache[i].flags & BUFFER_DIRTY) {
            block_write(dev, buffer_cache[i].sector, 1, buffer_cache[i].data);
            buffer_cache[i].flags &= ~BUFFER_DIRTY;
        }
    }
    
    block_flush(dev);
    spin_unlock(&buffer_lock);
}

/*============================================================================
 * REQUEST QUEUE
 *============================================================================*/

void init_request_queue(request_queue_t *q, uint32_t max_pending) {
    if (!q) return;
    
    q->head = NULL;
    q->tail = NULL;
    q->pending = 0;
    q->max_pending = max_pending;
}

void add_request(request_queue_t *q, bio_t *bio) {
    if (!q || !bio) return;
    
    bio->next = NULL;
    bio->prev = NULL;
    
    if (q->tail) {
        q->tail->next = bio;
        bio->prev = q->tail;
        q->tail = bio;
    } else {
        q->head = q->tail = bio;
    }
    
    q->pending++;
}

bio_t *get_request(request_queue_t *q) {
    if (!q || !q->head) return NULL;
    
    bio_t *bio = q->head;
    q->head = bio->next;
    
    if (q->head) {
        q->head->prev = NULL;
    } else {
        q->tail = NULL;
    }
    
    bio->next = NULL;
    bio->prev = NULL;
    q->pending--;
    
    return bio;
}

void process_requests(request_queue_t *q) {
    if (!q) return;
    
    while (q->head) {
        bio_t *bio = get_request(q);
        
        if (!bio) break;
        
        if (bio->flags & REQ_READ) {
            bio->error = block_read(bio->dev, bio->sector, bio->count, bio->buffer);
        } else if (bio->flags & REQ_WRITE) {
            bio->error = block_write(bio->dev, bio->sector, bio->count, bio->buffer);
        } else if (bio->flags & REQ_FLUSH) {
            bio->error = block_flush(bio->dev);
        }
        
        if (bio->callback) {
            bio->callback(bio);
        }
    }
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int ata_init(void) {
    console_print("Initializing ATA driver...\n");
    
    /* Detect devices */
    ata_detect_devices();
    
    /* Initialize buffer cache */
    buffer_cache_init();
    
    /* Register detected devices as block devices */
    for (int i = 0; i < 4; i++) {
        if (ata_devices[i].type == ATA_TYPE_ATA) {
            block_device_t *bdev = &block_devices[block_device_count];
            bdev->name = (i == 0) ? "hda" : (i == 1) ? "hdb" : (i == 2) ? "hdc" : "hdd";
            bdev->ops = ata_block_ops;
            bdev->total_sectors = ata_devices[i].sectors_48;
            bdev->sector_size = 512;
            bdev->type = 0;
            bdev->private_data = &ata_devices[i];
            
            register_block_device(bdev);
        }
    }
    
    console_print("ATA driver initialized\n");
    
    return 0;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

ata_stats_t *ata_get_stats(void) {
    return &ata_stats;
}

void ata_print_stats(void) {
    console_print("\n=== ATA Statistics ===\n");
    console_print("Reads: %lu (%lu bytes)\n", 
                 ata_stats.reads, ata_stats.read_bytes);
    console_print("Writes: %lu (%lu bytes)\n",
                 ata_stats.writes, ata_stats.write_bytes);
    console_print("Cache hits: %lu\n", ata_stats.cache_hits);
    console_print("Cache misses: %lu\n", ata_stats.cache_misses);
    console_print("DMA reads: %lu\n", ata_stats.dma_reads);
    console_print("DMA writes: %lu\n", ata_stats.dma_writes);
}
