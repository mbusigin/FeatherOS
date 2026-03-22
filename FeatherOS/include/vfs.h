/* FeatherOS - Virtual File System Header
 * Sprint 21: Virtual File System (VFS)
 */

#ifndef FEATHEROS_VFS_H
#define FEATHEROS_VFS_H

#include <stdint.h>
#include <stdbool.h>
#include <sync.h>

/*============================================================================
 * VFS CONSTANTS
 *============================================================================*/

#define MAX_PATH         256
#define MAX_FILENAME     255
#define MAX_FD           1024
#define MAX_MOUNTS       32
#define MAX_DENTRY_CACHE 256

/* File types */
#define VFS_TYPE_NONE     0
#define VFS_TYPE_REG      1   /* Regular file */
#define VFS_TYPE_DIR      2   /* Directory */
#define VFS_TYPE_CHAR     3   /* Character device */
#define VFS_TYPE_BLOCK    4   /* Block device */
#define VFS_TYPE_FIFO     5   /* Named pipe */
#define VFS_TYPE_SOCKET   6   /* Socket */
#define VFS_TYPE_LINK     7   /* Symbolic link */

/* File flags */
#define VFS_O_RDONLY      0x0000
#define VFS_O_WRONLY      0x0001
#define VFS_O_RDWR        0x0002
#define VFS_O_CREAT       0x0040
#define VFS_O_EXCL        0x0080
#define VFS_O_NOCTTY      0x0100
#define VFS_O_TRUNC       0x0200
#define VFS_O_APPEND      0x0400
#define VFS_O_NONBLOCK    0x0800
#define VFS_O_SYNC        0x1000

/* Seek modes */
#define VFS_SEEK_SET      0
#define VFS_SEEK_CUR      1
#define VFS_SEEK_END      2

/* Access modes */
#define VFS_MODE_RWX      0x1C0
#define VFS_MODE_RUSR     0x100
#define VFS_MODE_WUSR     0x080
#define VFS_MODE_XUSR     0x040
#define VFS_MODE_RGRP     0x020
#define VFS_MODE_WGRP     0x010
#define VFS_MODE_XGRP     0x008
#define VFS_MODE_ROTH     0x004
#define VFS_MODE_WOTH     0x002
#define VFS_MODE_XOTH     0x001

/*============================================================================
 * INODE
 *============================================================================*/

typedef struct vnode vnode_t;
typedef struct superblock superblock_t;
typedef struct file file_t;
typedef struct dentry dentry_t;
typedef struct mount mount_t;

struct vnode_operations;
struct file_operations;
struct dentry_operations;

/* Inode structure */
struct vnode {
    /* Basic info */
    uint32_t ino;           /* Inode number */
    uint32_t size;          /* File size */
    uint16_t mode;          /* File mode (type + permissions) */
    uint16_t nlink;         /* Hard link count */
    uint32_t uid;           /* Owner UID */
    uint32_t gid;           /* Owner GID */
    
    /* Timestamps */
    uint64_t atime;         /* Access time */
    uint64_t mtime;         /* Modification time */
    uint64_t ctime;         /* Change time */
    
    /* Device info (for device files) */
    uint32_t rdev;          /* Real device ID */
    
    /* Operations */
    struct vnode_operations *ops;
    struct file_operations *fops;
    
    /* Superblock reference */
    superblock_t *sb;
    
    /* Private data */
    void *data;
    
    /* Reference count */
    int refcount;
};

/* Vnode operations */
struct vnode_operations {
    int (*lookup)(vnode_t *parent, const char *name, vnode_t **result);
    int (*mkdir)(vnode_t *parent, const char *name, uint16_t mode);
    int (*rmdir)(vnode_t *parent, const char *name);
    int (*unlink)(vnode_t *parent, const char *name);
    int (*rename)(vnode_t *old_parent, const char *old_name,
                  vnode_t *new_parent, const char *new_name);
    int (*getattr)(vnode_t *vnode, void *attr);
    int (*setattr)(vnode_t *vnode, void *attr);
};

/* File operations */
struct file_operations {
    int (*open)(vnode_t *vnode, file_t *file);
    int (*close)(file_t *file);
    int (*read)(file_t *file, void *buf, uint32_t count, uint64_t *pos);
    int (*write)(file_t *file, const void *buf, uint32_t count, uint64_t *pos);
    int (*lseek)(file_t *file, uint64_t offset, int mode, uint64_t *result);
    int (*ioctl)(file_t *file, uint32_t cmd, void *arg);
    int (*readdir)(file_t *file, void *dirent, uint64_t *pos);
};

/*============================================================================
 * SUPERBLOCK
 *============================================================================*/

/* Filesystem operations */
typedef struct filesystem {
    const char *name;
    int (*mount)(superblock_t *sb, const char *device, void *data);
    int (*unmount)(superblock_t *sb);
    vnode_t *(*root)(superblock_t *sb);
    struct filesystem *next;
} filesystem_t;

/* Superblock structure */
struct superblock {
    uint32_t s_blocksize;      /* Block size */
    uint64_t s_blocks;          /* Total blocks */
    uint64_t s_free_blocks;     /* Free blocks */
    uint64_t s_free_inodes;     /* Free inodes */
    uint32_t s_magic;           /* Magic number */
    
    /* Filesystem operations */
    filesystem_t *fs;
    
    /* Root vnode */
    vnode_t *s_root;
    
    /* Device */
    char device[64];
    
    /* Private data */
    void *data;
    
    /* Reference count */
    int refcount;
};

/*============================================================================
 * DENTRY (Directory Entry)
 *============================================================================*/

/* Dentry operations */
struct dentry_operations {
    int (*d_hash)(dentry_t *dentry, uint32_t *hash);
    int (*d_compare)(dentry_t *parent, const char *name, vnode_t *vnode);
    int (*d_delete)(dentry_t *dentry);
    void (*d_release)(dentry_t *dentry);
};

/* Dentry structure */
struct dentry {
    char name[MAX_FILENAME + 1];  /* Name */
    vnode_t *vnode;               /* Associated vnode */
    dentry_t *parent;              /* Parent dentry */
    
    /* Children list */
    dentry_t *subdirs;
    dentry_t *next;
    dentry_t *prev;
    
    /* Operations */
    struct dentry_operations *dops;
    
    /* State flags */
    uint32_t flags;
    #define DENTRY_MOUNTED      0x0001  /* Mount point */
    #define DENTRY_NEGATIVE     0x0002  /* Negative entry */
    
    /* Hash linkage */
    uint32_t hash;
    
    /* Reference count */
    int refcount;
};

/*============================================================================
 * MOUNT
 *============================================================================*/

/* Mount structure */
struct mount {
    char source[64];       /* Mount source (device/path) */
    char target[256];      /* Mount point path */
    superblock_t *sb;      /* Superblock */
    dentry_t *mount_point; /* Mount point dentry */
    
    mount_t *next;
};

/*============================================================================
 * FILE
 *============================================================================*/

/* File structure */
struct file {
    vnode_t *vnode;           /* Associated vnode */
    uint64_t f_pos;           /* Current position */
    uint32_t f_flags;         /* Flags (O_RDONLY, etc.) */
    uint32_t f_mode;         /* Access mode */
    
    /* File operations */
    struct file_operations *fops;
    
    /* Private data */
    void *private_data;
    
    /* Reference count */
    int refcount;
};

/*============================================================================
 * PATH RESOLUTION
 *============================================================================*/

/* Path component info */
typedef struct {
    char name[MAX_FILENAME + 1];
    int len;
    bool is_dir;
} path_component_t;

/* VFS FUNCTIONS */

/* Initialize VFS */
int vfs_init(void);

/* Mount/unmount */
int vfs_mount(const char *source, const char *target, const char *fs_type, void *data);
int vfs_unmount(const char *target);

/* Register filesystem */
int register_filesystem(filesystem_t *fs);
int unregister_filesystem(const char *name);

/* Path operations */
int path_lookup(const char *path, int flags, vnode_t **vnode);
int path_resolve(const char *path, char *resolved, size_t len);

/* Directory operations */
int vfs_mkdir(const char *path, uint16_t mode);
int vfs_rmdir(const char *path);
int vfs_create(const char *path, uint16_t mode);
int vfs_unlink(const char *path);
int vfs_rename(const char *old_path, const char *new_path);

/* File operations */
int vfs_open(const char *path, int flags, file_t **file);
int vfs_close(file_t *file);
int vfs_read(file_t *file, void *buf, uint32_t count, uint32_t *read);
int vfs_write(file_t *file, const void *buf, uint32_t count, uint32_t *written);
int vfs_lseek(file_t *file, uint64_t offset, int mode, uint64_t *new_pos);

/* Directory reading */
int vfs_readdir(file_t *file, void *dirent, uint32_t *count);

/* Attribute operations */
int vfs_stat(vnode_t *vnode, void *stat_buf);
int vfs_fstat(file_t *file, void *stat_buf);

/* File descriptor table (per-process) */
typedef struct fd_table {
    file_t *files[MAX_FD];
    uint32_t flags[MAX_FD];
    int next_fd;
    spinlock_t lock;
} fd_table_t;

fd_table_t *fd_table_create(void);
void fd_table_destroy(fd_table_t *table);
int fd_table_alloc(fd_table_t *table, file_t *file);
int fd_table_free(fd_table_t *table, int fd);
file_t *fd_table_get(fd_table_t *table, int fd);

/* Dentry cache */
int dentry_cache_init(void);
dentry_t *dentry_lookup(const char *path);
int dentry_add(dentry_t *parent, const char *name, vnode_t *vnode);
int dentry_remove(dentry_t *dentry);
void dentry_release(dentry_t *dentry);
void dentry_prune(void);

/* Superblock operations */
superblock_t *superblock_get(vnode_t *vnode);
int superblock_put(superblock_t *sb);

/* Vnode operations */
vnode_t *vnode_get(vnode_t *vnode);
int vnode_put(vnode_t *vnode);

/* Utility functions */
const char *vfs_basename(const char *path);
const char *vfs_dirname(const char *path);
int vfs_path_join(char *dest, size_t len, const char *dir, const char *name);
bool vfs_is_absolute(const char *path);
bool vfs_path_equal(const char *a, const char *b);

#endif /* FEATHEROS_VFS_H */
