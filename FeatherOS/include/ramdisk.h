/* FeatherOS - RAM Disk & DevFS Header
 * Sprint 22: RAM Disk & DevFS
 */

#ifndef FEATHEROS_RAMDISK_H
#define FEATHEROS_RAMDISK_H

#include <stdint.h>
#include <stdbool.h>
#include <sync.h>
#include <process.h>

/*============================================================================
 * RAM DISK CONSTANTS
 *============================================================================*/

#define RAMDISK_MAX_DEVICES    8
#define RAMDISK_DEFAULT_SIZE   (16 * 1024 * 1024)  /* 16 MB */
#define RAMDISK_BLOCK_SIZE     512

/*============================================================================
 * RAM DISK DEVICE
 *============================================================================*/

typedef struct ramdisk {
    char name[32];
    uint8_t *data;
    uint32_t size;
    uint32_t block_count;
    int major;
    int minor;
    spinlock_t lock;
    bool registered;
} ramdisk_t;

/*============================================================================
 * RAM DISK OPERATIONS
 *============================================================================*/

/* Initialize RAM disk subsystem */
int ramdisk_init(void);

/* Create a new RAM disk */
ramdisk_t *ramdisk_create(const char *name, uint32_t size);

/* Delete a RAM disk */
int ramdisk_delete(ramdisk_t *dev);

/* Read from RAM disk */
int ramdisk_read(ramdisk_t *dev, uint32_t sector, uint32_t count, void *buffer);

/* Write to RAM disk */
int ramdisk_write(ramdisk_t *dev, uint32_t sector, uint32_t count, const void *buffer);

/* Get RAM disk by name */
ramdisk_t *ramdisk_get(const char *name);

/* Register RAM disk as block device */
int ramdisk_register(ramdisk_t *dev);

/* Unregister RAM disk */
int ramdisk_unregister(ramdisk_t *dev);

/* Get device count */
int ramdisk_get_count(void);

/* Get statistics */
void ramdisk_get_stats(uint64_t *total_size, uint64_t *used_size);

/*============================================================================
 * DEVFS (Device Filesystem)
 *============================================================================*/

/* Initialize devfs */
int devfs_init(void);

/* Create device node */
int devfs_mknod(const char *path, uint16_t mode, uint32_t dev);

/* Remove device node */
int devfs_unlink(const char *path);

/* Standard device numbers */
#define DEV_NULL   0x0101  /* /dev/null */
#define DEV_ZERO   0x0102  /* /dev/zero */
#define DEV_RANDOM 0x0103  /* /dev/random */
#define DEV_URANDOM 0x0104 /* /dev/urandom */
#define DEV_TTY    0x0105  /* /dev/tty */

/*============================================================================
 * DEVICE OPERATIONS
 *============================================================================*/

/* null device - reads return 0, writes discard */
int dev_null_read(void *buf, uint32_t count, uint32_t *read);
int dev_null_write(const void *buf, uint32_t count, uint32_t *written);

/* zero device - reads return zeros */
int dev_zero_read(void *buf, uint32_t count, uint32_t *read);

/* random device - reads return random bytes */
int dev_random_read(void *buf, uint32_t count, uint32_t *read);
int dev_urandom_read(void *buf, uint32_t count, uint32_t *read);

/*============================================================================
 * TMPFS (Temporary Filesystem)
 *============================================================================*/

#define TMPFS_MAX_SIZE   (256 * 1024 * 1024)  /* 256 MB max */
#define TMPFS_BLOCK_SIZE 4096

typedef struct tmpfs_inode {
    uint32_t ino;
    char name[256];
    uint32_t size;
    uint16_t mode;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
    uint8_t *data;
    struct tmpfs_inode *parent;
    struct tmpfs_inode *children;
    struct tmpfs_inode *next;
    struct tmpfs_inode *prev;
    int refcount;
} tmpfs_inode_t;

typedef struct tmpfs_sb {
    uint32_t next_ino;
    tmpfs_inode_t *root;
    uint32_t max_size;
    uint32_t used_size;
} tmpfs_sb_t;

/* Initialize tmpfs */
int tmpfs_init(void);

/* Create tmpfs mount */
int tmpfs_mount(const char *path);

/* Create tmpfs inode */
tmpfs_inode_t *tmpfs_create(tmpfs_inode_t *parent, const char *name, uint16_t mode);

/* Delete tmpfs inode */
int tmpfs_delete(tmpfs_inode_t *inode);

/* Read from tmpfs inode */
int tmpfs_read(tmpfs_inode_t *inode, uint32_t offset, void *buf, uint32_t count);

/* Write to tmpfs inode */
int tmpfs_write(tmpfs_inode_t *inode, uint32_t offset, const void *buf, uint32_t count);

/* Get tmpfs stats */
void tmpfs_get_stats(uint64_t *total, uint64_t *used, uint64_t *inodes);

/*============================================================================
 * FIFO (Named Pipe)
 *============================================================================*/

#define FIFO_MAX_SIZE    65536

typedef struct fifo {
    char name[256];
    uint8_t *buffer;
    uint32_t size;
    uint32_t read_pos;
    uint32_t write_pos;
    uint32_t data_len;
    spinlock_t lock;
    bool closed;
    int refcount;
    struct fifo *next;
} fifo_t;

/* Create FIFO */
fifo_t *fifo_create(const char *name);

/* Delete FIFO */
int fifo_delete(fifo_t *fifo);

/* Read from FIFO */
int fifo_read(fifo_t *fifo, void *buf, uint32_t count, uint32_t *read);

/* Write to FIFO */
int fifo_write(fifo_t *fifo, const void *buf, uint32_t count, uint32_t *written);

/* Get FIFO by name */
fifo_t *fifo_get(const char *name);

/*============================================================================
 * STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t ramdisk_reads;
    uint64_t ramdisk_writes;
    uint64_t ramdisk_read_bytes;
    uint64_t ramdisk_write_bytes;
    uint64_t dev_null_reads;
    uint64_t dev_null_writes;
    uint64_t dev_zero_reads;
    uint64_t dev_random_reads;
    uint64_t fifo_creates;
    uint64_t fifo_reads;
    uint64_t fifo_writes;
} ramdisk_stats_t;

/* Get all statistics */
ramdisk_stats_t *ramdisk_get_all_stats(void);

/* Print statistics */
void ramdisk_print_stats(void);

#endif /* FEATHEROS_RAMDISK_H */
