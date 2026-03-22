/* FeatherOS - RAM Disk & DevFS Implementation
 * Sprint 22: RAM Disk & DevFS
 */

#include <ramdisk.h>
#include <kernel.h>
#include <memory.h>
#include <datastructures.h>
#include <sync.h>
#include <timer.h>
#include <vfs.h>

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

static ramdisk_t *ramdisks[RAMDISK_MAX_DEVICES];
static int ramdisk_count = 0;
static spinlock_t ramdisk_lock;

static ramdisk_stats_t ramdisk_stats = {0};

/* /dev/null zero buffer (for reads) */
static uint8_t null_buffer[512];
static spinlock_t null_lock;

/* /dev/zero is just null_buffer (all zeros) */
/* /dev/random entropy buffer */
static uint8_t random_buffer[256];
static uint32_t random_pos = 0;
static spinlock_t random_lock;

/* FIFO list */
static fifo_t *fifos = NULL;
static spinlock_t fifo_lock;

/* Tmpfs superblock */
static tmpfs_sb_t *tmpfs_sb = NULL;

/*============================================================================
 * RAM DISK OPERATIONS
 *============================================================================*/

int ramdisk_init(void) {
    console_print("Initializing RAM disk subsystem...\n");
    
    for (int i = 0; i < RAMDISK_MAX_DEVICES; i++) {
        ramdisks[i] = NULL;
    }
    
    spin_lock_init(&ramdisk_lock);
    spin_lock_init(&null_lock);
    spin_lock_init(&random_lock);
    spin_lock_init(&fifo_lock);
    
    /* Initialize /dev/null buffer with zeros */
    memset(null_buffer, 0, sizeof(null_buffer));
    
    /* Initialize random buffer with pseudo-random data */
    for (int i = 0; i < 256; i++) {
        random_buffer[i] = (i * 17 + 42) & 0xFF;
    }
    
    /* Create default RAM disk */
    ramdisk_t *rd = ramdisk_create("ram0", RAMDISK_DEFAULT_SIZE);
    if (rd) {
        ramdisk_register(rd);
        console_print("ramdisk: Created %s (%d MB)\n", rd->name, rd->size / (1024 * 1024));
    }
    
    /* Initialize devfs */
    devfs_init();
    
    /* Initialize tmpfs */
    tmpfs_init();
    
    console_print("ramdisk: RAM disk subsystem initialized\n");
    
    return 0;
}

ramdisk_t *ramdisk_create(const char *name, uint32_t size) {
    if (!name || ramdisk_count >= RAMDISK_MAX_DEVICES) {
        return NULL;
    }
    
    ramdisk_t *dev = (ramdisk_t *)kmalloc(sizeof(ramdisk_t));
    if (!dev) return NULL;
    
    memset(dev, 0, sizeof(ramdisk_t));
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->size = size;
    dev->block_count = size / RAMDISK_BLOCK_SIZE;
    dev->minor = ramdisk_count;
    spin_lock_init(&dev->lock);
    
    /* Allocate data buffer */
    dev->data = (uint8_t *)kmalloc(size);
    if (!dev->data) {
        kfree(dev);
        return NULL;
    }
    
    memset(dev->data, 0, size);
    
    spin_lock(&ramdisk_lock);
    ramdisks[ramdisk_count++] = dev;
    spin_unlock(&ramdisk_lock);
    
    return dev;
}

int ramdisk_delete(ramdisk_t *dev) {
    if (!dev) return -1;
    
    spin_lock(&ramdisk_lock);
    
    for (int i = 0; i < RAMDISK_MAX_DEVICES; i++) {
        if (ramdisks[i] == dev) {
            ramdisks[i] = NULL;
            ramdisk_count--;
            break;
        }
    }
    
    spin_unlock(&ramdisk_lock);
    
    if (dev->data) {
        kfree(dev->data);
    }
    kfree(dev);
    
    return 0;
}

int ramdisk_read(ramdisk_t *dev, uint32_t sector, uint32_t count, void *buffer) {
    if (!dev || !buffer) return -1;
    
    spin_lock(&dev->lock);
    
    uint32_t offset = sector * RAMDISK_BLOCK_SIZE;
    if (offset + count * RAMDISK_BLOCK_SIZE > dev->size) {
        spin_unlock(&dev->lock);
        return -1;
    }
    
    memcpy(buffer, dev->data + offset, count * RAMDISK_BLOCK_SIZE);
    
    ramdisk_stats.ramdisk_reads++;
    ramdisk_stats.ramdisk_read_bytes += count * RAMDISK_BLOCK_SIZE;
    
    spin_unlock(&dev->lock);
    
    return 0;
}

int ramdisk_write(ramdisk_t *dev, uint32_t sector, uint32_t count, const void *buffer) {
    if (!dev || !buffer) return -1;
    
    spin_lock(&dev->lock);
    
    uint32_t offset = sector * RAMDISK_BLOCK_SIZE;
    if (offset + count * RAMDISK_BLOCK_SIZE > dev->size) {
        spin_unlock(&dev->lock);
        return -1;
    }
    
    memcpy(dev->data + offset, buffer, count * RAMDISK_BLOCK_SIZE);
    
    ramdisk_stats.ramdisk_writes++;
    ramdisk_stats.ramdisk_write_bytes += count * RAMDISK_BLOCK_SIZE;
    
    spin_unlock(&dev->lock);
    
    return 0;
}

ramdisk_t *ramdisk_get(const char *name) {
    if (!name) return NULL;
    
    spin_lock(&ramdisk_lock);
    
    for (int i = 0; i < RAMDISK_MAX_DEVICES; i++) {
        if (ramdisks[i] && strcmp(ramdisks[i]->name, name) == 0) {
            spin_unlock(&ramdisk_lock);
            return ramdisks[i];
        }
    }
    
    spin_unlock(&ramdisk_lock);
    return NULL;
}

int ramdisk_register(ramdisk_t *dev) {
    if (!dev) return -1;
    dev->registered = true;
    return 0;
}

int ramdisk_unregister(ramdisk_t *dev) {
    if (!dev) return -1;
    dev->registered = false;
    return 0;
}

int ramdisk_get_count(void) {
    return ramdisk_count;
}

void ramdisk_get_stats(uint64_t *total_size, uint64_t *used_size) {
    if (total_size) *total_size = 0;
    if (used_size) *used_size = 0;
    
    spin_lock(&ramdisk_lock);
    
    for (int i = 0; i < RAMDISK_MAX_DEVICES; i++) {
        if (ramdisks[i]) {
            if (total_size) *total_size += ramdisks[i]->size;
            if (used_size) *used_size += ramdisks[i]->size;
        }
    }
    
    spin_unlock(&ramdisk_lock);
}

/*============================================================================
 * DEV NULL/ZERO/RANDOM OPERATIONS
 *============================================================================*/

int dev_null_read(void *buf, uint32_t count, uint32_t *read) {
    if (!buf) return -1;
    
    spin_lock(&null_lock);
    memset(buf, 0, count);
    if (read) *read = count;
    ramdisk_stats.dev_null_reads++;
    spin_unlock(&null_lock);
    
    return 0;
}

int dev_null_write(const void *buf, uint32_t count, uint32_t *written) {
    if (!buf) return -1;
    
    spin_lock(&null_lock);
    if (written) *written = count;
    ramdisk_stats.dev_null_writes++;
    spin_unlock(&null_lock);
    
    return 0;
}

int dev_zero_read(void *buf, uint32_t count, uint32_t *read) {
    if (!buf) return -1;
    
    memset(buf, 0, count);
    if (read) *read = count;
    ramdisk_stats.dev_zero_reads++;
    
    return 0;
}

int dev_random_read(void *buf, uint32_t count, uint32_t *read) {
    if (!buf) return -1;
    
    spin_lock(&random_lock);
    
    uint8_t *p = (uint8_t *)buf;
    uint32_t i;
    for (i = 0; i < count; i++) {
        p[i] = random_buffer[random_pos];
        random_pos = (random_pos + 1) % 256;
    }
    
    if (read) *read = count;
    ramdisk_stats.dev_random_reads++;
    
    spin_unlock(&random_lock);
    
    return 0;
}

int dev_urandom_read(void *buf, uint32_t count, uint32_t *read) {
    return dev_random_read(buf, count, read);
}

/*============================================================================
 * DEVFS
 *============================================================================*/

int devfs_init(void) {
    console_print("devfs: Initializing device filesystem...\n");
    
    /* Create standard device nodes */
    /* These would be created via mknod in real implementation */
    console_print("devfs: /dev/null (character 1:3)\n");
    console_print("devfs: /dev/zero (character 1:5)\n");
    console_print("devfs: /dev/random (character 1:8)\n");
    console_print("devfs: /dev/urandom (character 1:9)\n");
    console_print("devfs: /dev/tty (character 5:0)\n");
    console_print("devfs: /dev/loop0 (block 7:0)\n");
    console_print("devfs: Device filesystem initialized\n");
    
    return 0;
}

int devfs_mknod(const char *path, uint16_t mode, uint32_t dev) {
    (void)path;
    (void)mode;
    (void)dev;
    return 0;  /* Simplified - would create actual node */
}

int devfs_unlink(const char *path) {
    (void)path;
    return 0;  /* Simplified */
}

/*============================================================================
 * TMPFS
 *============================================================================*/

int tmpfs_init(void) {
    console_print("tmpfs: Initializing temporary filesystem...\n");
    
    tmpfs_sb = (tmpfs_sb_t *)kmalloc(sizeof(tmpfs_sb_t));
    if (!tmpfs_sb) return -1;
    
    memset(tmpfs_sb, 0, sizeof(tmpfs_sb_t));
    tmpfs_sb->next_ino = 1;
    tmpfs_sb->max_size = TMPFS_MAX_SIZE;
    tmpfs_sb->used_size = 0;
    
    /* Create root inode */
    tmpfs_inode_t *root = (tmpfs_inode_t *)kmalloc(sizeof(tmpfs_inode_t));
    if (!root) {
        kfree(tmpfs_sb);
        return -1;
    }
    
    memset(root, 0, sizeof(tmpfs_inode_t));
    root->ino = tmpfs_sb->next_ino++;
    strcpy(root->name, "/");
    root->mode = VFS_TYPE_DIR | VFS_MODE_RWX;
    root->refcount = 1;
    
    tmpfs_sb->root = root;
    
    console_print("tmpfs: Temporary filesystem initialized (max size: %d MB)\n",
                 TMPFS_MAX_SIZE / (1024 * 1024));
    
    return 0;
}

int tmpfs_mount(const char *path) {
    if (!path) return -1;
    
    /* Would mount tmpfs at path */
    console_print("tmpfs: Mounted at %s\n", path);
    
    return 0;
}

tmpfs_inode_t *tmpfs_create(tmpfs_inode_t *parent, const char *name, uint16_t mode) {
    if (!name) return NULL;
    
    tmpfs_inode_t *inode = (tmpfs_inode_t *)kmalloc(sizeof(tmpfs_inode_t));
    if (!inode) return NULL;
    
    memset(inode, 0, sizeof(tmpfs_inode_t));
    inode->ino = tmpfs_sb->next_ino++;
    strncpy(inode->name, name, sizeof(inode->name) - 1);
    inode->mode = mode;
    inode->parent = parent;
    inode->refcount = 1;
    inode->atime = inode->mtime = inode->ctime = get_uptime_ms();
    
    /* Add to parent's children */
    if (parent) {
        inode->next = parent->children;
        if (parent->children) {
            parent->children->prev = inode;
        }
        parent->children = inode;
    }
    
    return inode;
}

int tmpfs_delete(tmpfs_inode_t *inode) {
    if (!inode) return -1;
    
    inode->refcount--;
    if (inode->refcount > 0) {
        return 0;
    }
    
    /* Remove from parent */
    if (inode->parent) {
        if (inode->prev) {
            inode->prev->next = inode->next;
        } else {
            inode->parent->children = inode->next;
        }
        if (inode->next) {
            inode->next->prev = inode->prev;
        }
    }
    
    /* Free data if regular file */
    if (inode->data) {
        tmpfs_sb->used_size -= inode->size;
        kfree(inode->data);
    }
    
    kfree(inode);
    
    return 0;
}

int tmpfs_read(tmpfs_inode_t *inode, uint32_t offset, void *buf, uint32_t count) {
    if (!inode || !buf) return -1;
    
    if (offset + count > inode->size) {
        count = inode->size - offset;
    }
    
    if (count == 0) return 0;
    
    memcpy(buf, inode->data + offset, count);
    
    return count;
}

int tmpfs_write(tmpfs_inode_t *inode, uint32_t offset, const void *buf, uint32_t count) {
    if (!inode || !buf) return -1;
    
    /* Expand buffer if needed */
    if (offset + count > inode->size) {
        uint32_t new_size = offset + count;
        
        /* Check size limit */
        if (tmpfs_sb->used_size + new_size > tmpfs_sb->max_size) {
            return -1;
        }
        
        uint8_t *new_data = (uint8_t *)kmalloc(new_size);
        if (!new_data) return -1;
        
        /* Copy old data */
        if (inode->data) {
            memcpy(new_data, inode->data, inode->size);
            tmpfs_sb->used_size -= inode->size;
            kfree(inode->data);
        }
        
        inode->data = new_data;
        inode->size = new_size;
        tmpfs_sb->used_size += new_size;
    }
    
    memcpy(inode->data + offset, buf, count);
    inode->mtime = get_uptime_ms();
    
    return count;
}

void tmpfs_get_stats(uint64_t *total, uint64_t *used, uint64_t *inodes) {
    if (total) *total = tmpfs_sb ? tmpfs_sb->max_size : 0;
    if (used) *used = tmpfs_sb ? tmpfs_sb->used_size : 0;
    if (inodes) *inodes = tmpfs_sb ? tmpfs_sb->next_ino : 0;
}

/*============================================================================
 * FIFO (Named Pipe)
 *============================================================================*/

fifo_t *fifo_create(const char *name) {
    if (!name) return NULL;
    
    fifo_t *fifo = (fifo_t *)kmalloc(sizeof(fifo_t));
    if (!fifo) return NULL;
    
    memset(fifo, 0, sizeof(fifo_t));
    strncpy(fifo->name, name, sizeof(fifo->name) - 1);
    fifo->size = FIFO_MAX_SIZE;
    fifo->buffer = (uint8_t *)kmalloc(fifo->size);
    
    if (!fifo->buffer) {
        kfree(fifo);
        return NULL;
    }
    
    spin_lock_init(&fifo->lock);
    fifo->refcount = 1;
    
    /* Add to list */
    spin_lock(&fifo_lock);
    fifo->next = fifos;
    fifos = fifo;
    spin_unlock(&fifo_lock);
    
    ramdisk_stats.fifo_creates++;
    
    return fifo;
}

int fifo_delete(fifo_t *fifo) {
    if (!fifo) return -1;
    
    fifo->closed = true;
    fifo->refcount--;
    
    if (fifo->refcount <= 0) {
        /* Remove from list */
        spin_lock(&fifo_lock);
        fifo_t **p = &fifos;
        while (*p) {
            if (*p == fifo) {
                *p = fifo->next;
                break;
            }
            p = &(*p)->next;
        }
        spin_unlock(&fifo_lock);
        
        if (fifo->buffer) kfree(fifo->buffer);
        kfree(fifo);
    }
    
    return 0;
}

int fifo_read(fifo_t *fifo, void *buf, uint32_t count, uint32_t *read) {
    if (!fifo || !buf) return -1;
    
    spin_lock(&fifo->lock);
    
    /* Wait for data if empty */
    while (fifo->data_len == 0) {
        if (fifo->closed) {
            spin_unlock(&fifo->lock);
            if (read) *read = 0;
            return 0;
        }
        spin_unlock(&fifo->lock);
        delay_ms(10);
        spin_lock(&fifo->lock);
    }
    
    /* Read data */
    uint32_t to_read = count < fifo->data_len ? count : fifo->data_len;
    uint32_t first_part = fifo->size - fifo->read_pos;
    
    if (first_part >= to_read) {
        memcpy(buf, fifo->buffer + fifo->read_pos, to_read);
        fifo->read_pos = (fifo->read_pos + to_read) % fifo->size;
    } else {
        memcpy(buf, fifo->buffer + fifo->read_pos, first_part);
        memcpy((uint8_t *)buf + first_part, fifo->buffer, to_read - first_part);
        fifo->read_pos = to_read - first_part;
    }
    
    fifo->data_len -= to_read;
    
    if (read) *read = to_read;
    ramdisk_stats.fifo_reads++;
    
    spin_unlock(&fifo->lock);
    
    return 0;
}

int fifo_write(fifo_t *fifo, const void *buf, uint32_t count, uint32_t *written) {
    if (!fifo || !buf) return -1;
    
    if (fifo->closed) return -1;
    
    spin_lock(&fifo->lock);
    
    /* Wait for space if full */
    while (fifo->data_len == fifo->size) {
        spin_unlock(&fifo->lock);
        delay_ms(10);
        spin_lock(&fifo->lock);
        if (fifo->closed) {
            spin_unlock(&fifo->lock);
            return -1;
        }
    }
    
    /* Write data */
    uint32_t to_write = count;
    if (to_write > fifo->size - fifo->data_len) {
        to_write = fifo->size - fifo->data_len;
    }
    
    uint32_t first_part = fifo->size - fifo->write_pos;
    
    if (first_part >= to_write) {
        memcpy(fifo->buffer + fifo->write_pos, buf, to_write);
        fifo->write_pos = (fifo->write_pos + to_write) % fifo->size;
    } else {
        memcpy(fifo->buffer + fifo->write_pos, buf, first_part);
        memcpy(fifo->buffer, (uint8_t *)buf + first_part, to_write - first_part);
        fifo->write_pos = to_write - first_part;
    }
    
    fifo->data_len += to_write;
    
    if (written) *written = to_write;
    ramdisk_stats.fifo_writes++;
    
    spin_unlock(&fifo->lock);
    
    return 0;
}

fifo_t *fifo_get(const char *name) {
    if (!name) return NULL;
    
    spin_lock(&fifo_lock);
    
    fifo_t *fifo = fifos;
    while (fifo) {
        if (strcmp(fifo->name, name) == 0) {
            fifo->refcount++;
            spin_unlock(&fifo_lock);
            return fifo;
        }
        fifo = fifo->next;
    }
    
    spin_unlock(&fifo_lock);
    return NULL;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

ramdisk_stats_t *ramdisk_get_all_stats(void) {
    return &ramdisk_stats;
}

void ramdisk_print_stats(void) {
    console_print("\n=== RAM Disk & DevFS Statistics ===\n");
    console_print("RAM Disk:\n");
    console_print("  Reads: %lu (%lu bytes)\n",
                 ramdisk_stats.ramdisk_reads, ramdisk_stats.ramdisk_read_bytes);
    console_print("  Writes: %lu (%lu bytes)\n",
                 ramdisk_stats.ramdisk_writes, ramdisk_stats.ramdisk_write_bytes);
    console_print("Device Operations:\n");
    console_print("  /dev/null: %lu reads, %lu writes\n",
                 ramdisk_stats.dev_null_reads, ramdisk_stats.dev_null_writes);
    console_print("  /dev/zero: %lu reads\n", ramdisk_stats.dev_zero_reads);
    console_print("  /dev/random: %lu reads\n", ramdisk_stats.dev_random_reads);
    console_print("FIFOs:\n");
    console_print("  Creates: %lu\n", ramdisk_stats.fifo_creates);
    console_print("  Reads: %lu\n", ramdisk_stats.fifo_reads);
    console_print("  Writes: %lu\n", ramdisk_stats.fifo_writes);
}

/*============================================================================
 * BENCHMARK
 *============================================================================*/

int ramdisk_benchmark(void) {
    console_print("ramdisk: Running benchmark...\n");
    
    ramdisk_t *rd = ramdisk_get("ram0");
    if (!rd) {
        console_print("ramdisk: No RAM disk available\n");
        return -1;
    }
    
    uint8_t buffer[512];
    uint32_t sectors = rd->size / 512;
    
    /* Sequential write test */
    uint64_t start = get_uptime_ms();
    for (uint32_t i = 0; i < sectors; i++) {
        ramdisk_write(rd, i, 1, buffer);
    }
    uint64_t write_time = get_uptime_ms() - start;
    
    /* Sequential read test */
    start = get_uptime_ms();
    for (uint32_t i = 0; i < sectors; i++) {
        ramdisk_read(rd, i, 1, buffer);
    }
    uint64_t read_time = get_uptime_ms() - start;
    
    /* Calculate throughput */
    uint64_t size_mb = rd->size / (1024 * 1024);
    uint64_t read_mbps = write_time > 0 ? (size_mb * 1000) / write_time : 0;
    uint64_t write_mbps = read_time > 0 ? (size_mb * 1000) / read_time : 0;
    
    console_print("ramdisk: %lu MB read in %lu ms (%lu MB/s)\n",
                 size_mb, read_time, read_mbps);
    console_print("ramdisk: %lu MB written in %lu ms (%lu MB/s)\n",
                 size_mb, write_time, write_mbps);
    
    return 0;
}
