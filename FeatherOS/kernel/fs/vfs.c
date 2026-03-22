/* FeatherOS - Virtual File System Implementation
 * Sprint 21: Virtual File System (VFS)
 */

#include <vfs.h>
#include <kernel.h>
#include <datastructures.h>
#include <sync.h>

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

static filesystem_t *filesystems = NULL;
static mount_t *mounts = NULL;
static int mount_count = 0;
static spinlock_t mount_lock;

static dentry_t *dentry_cache = NULL;
static int dentry_cache_count = 0;
static spinlock_t dentry_cache_lock;

/* Root vnode and dentry */
static vnode_t *root_vnode = NULL;
static dentry_t *root_dentry = NULL;

/* Statistics */
static uint64_t vfs_lookups = 0;
static uint64_t vfs_cache_hits = 0;
static uint64_t vfs_operations = 0;

/*============================================================================
 * PATH UTILITIES
 *============================================================================*/

const char *vfs_basename(const char *path) {
    if (!path) return ".";
    
    const char *last = path + strlen(path) - 1;
    while (last > path && *last == '/') last--;
    
    if (last == path && *last == '/') return "/";
    
    const char *base = last;
    while (base > path && *(base - 1) != '/') base--;
    
    return base;
}

const char *vfs_dirname(const char *path) {
    static char dirname_buf[MAX_PATH];
    if (!path) return ".";
    
    const char *last = path + strlen(path) - 1;
    while (last > path && *last == '/') last--;
    
    while (last > path && *(last - 1) != '/') last--;
    
    if (last == path) {
        if (*last == '/') {
            return "/";
        }
        return ".";
    }
    
    size_t len = last - path;
    if (len >= MAX_PATH) len = MAX_PATH - 1;
    
    strncpy(dirname_buf, path, len);
    dirname_buf[len] = '\0';
    
    return dirname_buf;
}

bool vfs_is_absolute(const char *path) {
    return path && path[0] == '/';
}

int vfs_path_join(char *dest, size_t len, const char *dir, const char *name) {
    if (!dest || !name) return -1;
    
    size_t pos = 0;
    
    if (dir) {
        size_t dlen = strlen(dir);
        if (dlen >= len) return -1;
        
        strcpy(dest, dir);
        pos = dlen;
        
        /* Add trailing slash if needed */
        if (pos > 0 && dest[pos - 1] != '/') {
            if (pos + 1 >= len) return -1;
            dest[pos++] = '/';
        }
    }
    
    size_t nlen = strlen(name);
    if (pos + nlen >= len) return -1;
    
    memcpy(dest + pos, name, nlen);
    dest[pos + nlen] = '\0';
    
    return 0;
}

bool vfs_path_equal(const char *a, const char *b) {
    if (!a || !b) return false;
    
    /* Skip leading slashes */
    while (*a == '/') a++;
    while (*b == '/') b++;
    
    return strcmp(a, b) == 0;
}

/*============================================================================
 * FILESYSTEM REGISTRATION
 *============================================================================*/

int register_filesystem(filesystem_t *fs) {
    if (!fs || !fs->name) return -1;
    
    filesystem_t **p = &filesystems;
    while (*p) {
        if (strcmp((*p)->name, fs->name) == 0) {
            return -1;  /* Already registered */
        }
        p = &(*p)->next;
    }
    
    fs->next = NULL;
    *p = fs;
    
    console_print("vfs: Registered filesystem '%s'\n", fs->name);
    
    return 0;
}

int unregister_filesystem(const char *name) {
    if (!name) return -1;
    
    filesystem_t **p = &filesystems;
    while (*p) {
        if (strcmp((*p)->name, name) == 0) {
            *p = (*p)->next;
            return 0;
        }
        p = &(*p)->next;
    }
    
    return -1;
}

filesystem_t *filesystem_get(const char *name) {
    if (!name) return NULL;
    
    filesystem_t *fs = filesystems;
    while (fs) {
        if (strcmp(fs->name, name) == 0) {
            return fs;
        }
        fs = fs->next;
    }
    
    return NULL;
}

/*============================================================================
 * MOUNT OPERATIONS
 *============================================================================*/

int vfs_mount(const char *source, const char *target, const char *fs_type, void *data) {
    if (!target || !fs_type) return -1;
    
    spin_lock(&mount_lock);
    
    /* Check if already mounted at target */
    mount_t *m = mounts;
    while (m) {
        if (strcmp(m->target, target) == 0) {
            spin_unlock(&mount_lock);
            return -1;  /* Already mounted */
        }
        m = m->next;
    }
    
    /* Get filesystem */
    filesystem_t *fs = filesystem_get(fs_type);
    if (!fs) {
        spin_unlock(&mount_lock);
        console_print("vfs: Unknown filesystem '%s'\n", fs_type);
        return -1;
    }
    
    /* Allocate superblock */
    superblock_t *sb = (superblock_t *)kmalloc(sizeof(superblock_t));
    if (!sb) {
        spin_unlock(&mount_lock);
        return -1;
    }
    
    memset(sb, 0, sizeof(superblock_t));
    strncpy(sb->device, source ? source : "", sizeof(sb->device) - 1);
    sb->fs = fs;
    
    /* Mount the filesystem */
    if (fs->mount(sb, source, data) != 0) {
        kfree(sb);
        spin_unlock(&mount_lock);
        console_print("vfs: Mount failed for '%s'\n", fs_type);
        return -1;
    }
    
    /* Get root vnode */
    if (!sb->s_root) {
        sb->s_root = fs->root(sb);
    }
    
    if (!sb->s_root) {
        if (fs->unmount) fs->unmount(sb);
        kfree(sb);
        spin_unlock(&mount_lock);
        return -1;
    }
    
    sb->s_root->sb = sb;
    
    /* Create mount structure */
    mount_t *mount = (mount_t *)kmalloc(sizeof(mount_t));
    if (!mount) {
        sb->s_root->refcount--;
        if (fs->unmount) fs->unmount(sb);
        kfree(sb);
        spin_unlock(&mount_lock);
        return -1;
    }
    
    memset(mount, 0, sizeof(mount_t));
    strncpy(mount->source, source ? source : "", sizeof(mount->source) - 1);
    strncpy(mount->target, target, sizeof(mount->target) - 1);
    mount->sb = sb;
    
    /* Add to mount list */
    mount->next = mounts;
    mounts = mount;
    mount_count++;
    
    spin_unlock(&mount_lock);
    
    console_print("vfs: Mounted '%s' at '%s' (type: %s)\n", 
                 source ? source : "none", target, fs_type);
    
    return 0;
}

int vfs_unmount(const char *target) {
    if (!target) return -1;
    
    spin_lock(&mount_lock);
    
    mount_t **p = &mounts;
    while (*p) {
        if (strcmp((*p)->target, target) == 0) {
            mount_t *m = *p;
            *p = m->next;
            mount_count--;
            
            /* Unmount filesystem */
            if (m->sb->fs->unmount) {
                m->sb->fs->unmount(m->sb);
            }
            
            /* Release root vnode */
            if (m->sb->s_root) {
                m->sb->s_root->refcount--;
            }
            
            kfree(m);
            spin_unlock(&mount_lock);
            
            console_print("vfs: Unmounted '%s'\n", target);
            return 0;
        }
        p = &(*p)->next;
    }
    
    spin_unlock(&mount_lock);
    return -1;
}

mount_t *vfs_find_mount(const char *path) {
    if (!path) return NULL;
    
    mount_t *best = NULL;
    size_t best_len = 0;
    
    mount_t *m = mounts;
    while (m) {
        size_t len = strlen(m->target);
        if (len > best_len && strncmp(path, m->target, len) == 0) {
            best = m;
            best_len = len;
        }
        m = m->next;
    }
    
    return best;
}

/*============================================================================
 * PATH RESOLUTION
 *============================================================================*/

int path_lookup(const char *path, int flags, vnode_t **result) {
    (void)flags;  /* Unused for now */
    if (!path || !result) return -1;
    
    vfs_lookups++;
    vfs_operations++;
    
    /* Check dentry cache first */
    dentry_t *cached = dentry_lookup(path);
    if (cached && cached->vnode) {
        vfs_cache_hits++;
        vnode_get(cached->vnode);
        *result = cached->vnode;
        return 0;
    }
    
    /* Find mount point */
    mount_t *mnt = vfs_find_mount(path);
    vnode_t *current;
    
    if (mnt) {
        current = mnt->sb->s_root;
        path = path + strlen(mnt->target);
        while (*path == '/') path++;
    } else {
        current = root_vnode;
    }
    
    if (!current) return -1;
    
    vnode_get(current);
    
    /* Handle root path */
    if (strcmp(path, "/") == 0 || strcmp(path, "") == 0) {
        *result = current;
        return 0;
    }
    
    /* Parse path components */
    char component[MAX_FILENAME + 1];
    const char *p = path;
    
    while (*p) {
        /* Skip slashes */
        while (*p == '/') p++;
        if (!*p) break;
        
        /* Extract component */
        size_t len = 0;
        while (*p && *p != '/' && len < MAX_FILENAME) {
            component[len++] = *p++;
        }
        component[len] = '\0';
        
        if (strcmp(component, ".") == 0) {
            continue;
        }
        
        if (strcmp(component, "..") == 0) {
            /* Go to parent */
            continue;
        }
        
        /* Lookup component */
        vnode_t *next = NULL;
        if (current->ops && current->ops->lookup) {
            int ret = current->ops->lookup(current, component, &next);
            if (ret != 0) {
                vnode_put(current);
                return ret;
            }
        }
        
        vnode_put(current);
        current = next;
        
        if (!current) {
            return -1;
        }
    }
    
    /* Cache the result */
    if (current) {
        dentry_add(NULL, path, current);
    }
    
    *result = current;
    return 0;
}

int path_resolve(const char *path, char *resolved, size_t len) {
    if (!path || !resolved) return -1;
    
    resolved[0] = '\0';
    
    /* Find mount point */
    mount_t *mnt = vfs_find_mount(path);
    
    if (mnt) {
        strncpy(resolved, mnt->target, len - 1);
        resolved[len - 1] = '\0';
        size_t pos = strlen(resolved);
        
        path = path + strlen(mnt->target);
        while (*path == '/') path++;
        
        if (*path && pos < len - 1) {
            if (resolved[pos - 1] != '/') {
                resolved[pos++] = '/';
                resolved[pos] = '\0';
            }
            size_t remaining = len - pos - 1;
            size_t path_len = strlen(path);
            if (path_len > remaining) path_len = remaining;
            memcpy(resolved + pos, path, path_len);
            resolved[pos + path_len] = '\0';
        }
    } else {
        strncpy(resolved, path, len - 1);
        resolved[len - 1] = '\0';
    }
    
    return 0;
}

/*============================================================================
 * FILE OPERATIONS
 *============================================================================*/

int vfs_open(const char *path, int flags, file_t **file) {
    if (!path || !file) return -1;
    
    vfs_operations++;
    
    /* Look up the path */
    vnode_t *vnode = NULL;
    int ret = path_lookup(path, flags, &vnode);
    
    if (ret != 0) {
        /* File doesn't exist - try to create if O_CREAT */
        if (flags & VFS_O_CREAT) {
            /* Would create file here */
            return -1;
        }
        return -1;
    }
    
    /* Allocate file structure */
    file_t *f = (file_t *)kmalloc(sizeof(file_t));
    if (!f) {
        vnode_put(vnode);
        return -1;
    }
    
    memset(f, 0, sizeof(file_t));
    f->vnode = vnode;
    f->f_pos = 0;
    f->f_flags = flags;
    f->refcount = 1;
    
    /* Get file operations */
    if (vnode->fops) {
        f->fops = vnode->fops;
    }
    
    /* Call open if present */
    if (f->fops && f->fops->open) {
        ret = f->fops->open(vnode, f);
        if (ret != 0) {
            kfree(f);
            vnode_put(vnode);
            return ret;
        }
    }
    
    /* Truncate if requested */
    if ((flags & VFS_O_TRUNC) && vnode->size > 0) {
        vnode->size = 0;
    }
    
    *file = f;
    
    return 0;
}

int vfs_close(file_t *file) {
    if (!file) return -1;
    
    file->refcount--;
    if (file->refcount > 0) {
        return 0;
    }
    
    /* Call close if present */
    if (file->fops && file->fops->close) {
        file->fops->close(file);
    }
    
    /* Release vnode */
    if (file->vnode) {
        vnode_put(file->vnode);
    }
    
    kfree(file);
    
    return 0;
}

int vfs_read(file_t *file, void *buf, uint32_t count, uint32_t *read) {
    if (!file || !buf) return -1;
    
    if (read) *read = 0;
    
    if (!(file->f_flags & (VFS_O_RDONLY | VFS_O_RDWR))) {
        return -1;
    }
    
    if (!file->fops || !file->fops->read) {
        return -1;
    }
    
    uint64_t pos = file->f_pos;
    int ret = file->fops->read(file, buf, count, &pos);
    
    if (ret >= 0) {
        file->f_pos = pos;
        if (read) *read = (uint32_t)ret;
    }
    
    vfs_operations++;
    return ret;
}

int vfs_write(file_t *file, const void *buf, uint32_t count, uint32_t *written) {
    if (!file || !buf) return -1;
    
    if (written) *written = 0;
    
    if (!(file->f_flags & (VFS_O_WRONLY | VFS_O_RDWR))) {
        return -1;
    }
    
    if (!file->fops || !file->fops->write) {
        return -1;
    }
    
    uint64_t pos = file->f_pos;
    int ret = file->fops->write(file, buf, count, &pos);
    
    if (ret >= 0) {
        file->f_pos = pos;
        if (written) *written = (uint32_t)ret;
    }
    
    vfs_operations++;
    return ret;
}

int vfs_lseek(file_t *file, uint64_t offset, int mode, uint64_t *new_pos) {
    if (!file) return -1;
    
    uint64_t new_offset = 0;
    
    switch (mode) {
        case VFS_SEEK_SET:
            new_offset = offset;
            break;
        case VFS_SEEK_CUR:
            new_offset = file->f_pos + offset;
            break;
        case VFS_SEEK_END:
            if (file->vnode) {
                new_offset = file->vnode->size + offset;
            }
            break;
        default:
            return -1;
    }
    
    file->f_pos = new_offset;
    
    if (new_pos) {
        *new_pos = new_offset;
    }
    
    return 0;
}

int vfs_readdir(file_t *file, void *dirent, uint32_t *count) {
    if (!file || !dirent) return -1;
    
    if (!file->fops || !file->fops->readdir) {
        return -1;
    }
    
    uint64_t pos = file->f_pos;
    int ret = file->fops->readdir(file, dirent, &pos);
    
    if (ret >= 0) {
        file->f_pos = pos;
        if (count) *count = (uint32_t)ret;
    }
    
    return ret;
}

/*============================================================================
 * VNODE OPERATIONS
 *============================================================================*/

vnode_t *vnode_get(vnode_t *vnode) {
    if (!vnode) return NULL;
    vnode->refcount++;
    return vnode;
}

int vnode_put(vnode_t *vnode) {
    if (!vnode) return -1;
    
    vnode->refcount--;
    if (vnode->refcount > 0) {
        return 0;
    }
    
    /* Free vnode */
    kfree(vnode);
    
    return 0;
}

/*============================================================================
 * SUPERBLOCK OPERATIONS
 *============================================================================*/

superblock_t *superblock_get(vnode_t *vnode) {
    if (!vnode || !vnode->sb) return NULL;
    vnode->sb->refcount++;
    return vnode->sb;
}

int superblock_put(superblock_t *sb) {
    if (!sb) return -1;
    sb->refcount--;
    return 0;
}

/*============================================================================
 * FILE DESCRIPTOR TABLE
 *============================================================================*/

fd_table_t *fd_table_create(void) {
    fd_table_t *table = (fd_table_t *)kmalloc(sizeof(fd_table_t));
    if (!table) return NULL;
    
    memset(table, 0, sizeof(fd_table_t));
    table->next_fd = 3;  /* 0, 1, 2 are stdin, stdout, stderr */
    spin_lock_init(&table->lock);
    
    return table;
}

void fd_table_destroy(fd_table_t *table) {
    if (!table) return;
    
    for (int i = 0; i < MAX_FD; i++) {
        if (table->files[i]) {
            vfs_close(table->files[i]);
        }
    }
    
    kfree(table);
}

int fd_table_alloc(fd_table_t *table, file_t *file) {
    if (!table || !file) return -1;
    
    spin_lock(&table->lock);
    
    int fd = -1;
    for (int i = 0; i < MAX_FD; i++) {
        if (!table->files[i]) {
            fd = i;
            break;
        }
    }
    
    if (fd < 0) {
        spin_unlock(&table->lock);
        return -1;
    }
    
    table->files[fd] = file;
    file->refcount++;
    
    spin_unlock(&table->lock);
    
    return fd;
}

int fd_table_free(fd_table_t *table, int fd) {
    if (!table || fd < 0 || fd >= MAX_FD) return -1;
    
    spin_lock(&table->lock);
    
    if (!table->files[fd]) {
        spin_unlock(&table->lock);
        return -1;
    }
    
    vfs_close(table->files[fd]);
    table->files[fd] = NULL;
    
    spin_unlock(&table->lock);
    
    return 0;
}

file_t *fd_table_get(fd_table_t *table, int fd) {
    if (!table || fd < 0 || fd >= MAX_FD) return NULL;
    
    spin_lock(&table->lock);
    file_t *file = table->files[fd];
    if (file) {
        file->refcount++;
    }
    spin_unlock(&table->lock);
    
    return file;
}

/*============================================================================
 * DENTRY CACHE
 *============================================================================*/

int dentry_cache_init(void) {
    dentry_cache = NULL;
    dentry_cache_count = 0;
    spin_lock_init(&dentry_cache_lock);
    return 0;
}

dentry_t *dentry_lookup(const char *path) {
    if (!path) return NULL;
    
    spin_lock(&dentry_cache_lock);
    
    dentry_t *d = dentry_cache;
    while (d) {
        if (strcmp(d->name, path) == 0 && !(d->flags & DENTRY_NEGATIVE)) {
            d->refcount++;
            spin_unlock(&dentry_cache_lock);
            return d;
        }
        d = d->next;
    }
    
    spin_unlock(&dentry_cache_lock);
    return NULL;
}

int dentry_add(dentry_t *parent, const char *name, vnode_t *vnode) {
    if (!name) return -1;
    
    spin_lock(&dentry_cache_lock);
    
    /* Check if already exists */
    dentry_t *d = dentry_cache;
    while (d) {
        if (strcmp(d->name, name) == 0) {
            spin_unlock(&dentry_cache_lock);
            return -1;
        }
        d = d->next;
    }
    
    /* Create new dentry */
    d = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (!d) {
        spin_unlock(&dentry_cache_lock);
        return -1;
    }
    
    memset(d, 0, sizeof(dentry_t));
    strncpy(d->name, name, MAX_FILENAME);
    d->name[MAX_FILENAME] = '\0';
    d->vnode = vnode;
    d->parent = parent;
    d->refcount = 1;
    
    /* Add to cache */
    d->next = dentry_cache;
    if (dentry_cache) {
        dentry_cache->prev = d;
    }
    dentry_cache = d;
    dentry_cache_count++;
    
    /* Prune if too large */
    if (dentry_cache_count > MAX_DENTRY_CACHE) {
        dentry_prune();
    }
    
    spin_unlock(&dentry_cache_lock);
    
    return 0;
}

int dentry_remove(dentry_t *dentry) {
    if (!dentry) return -1;
    
    spin_lock(&dentry_cache_lock);
    
    if (dentry->prev) {
        dentry->prev->next = dentry->next;
    } else {
        dentry_cache = dentry->next;
    }
    
    if (dentry->next) {
        dentry->next->prev = dentry->prev;
    }
    
    dentry_cache_count--;
    
    spin_unlock(&dentry_cache_lock);
    
    kfree(dentry);
    
    return 0;
}

void dentry_release(dentry_t *dentry) {
    if (!dentry) return;
    
    spin_lock(&dentry_cache_lock);
    
    dentry->refcount--;
    if (dentry->refcount <= 0) {
        /* Remove from cache and free */
        if (dentry->prev) {
            dentry->prev->next = dentry->next;
        } else {
            dentry_cache = dentry->next;
        }
        
        if (dentry->next) {
            dentry->next->prev = dentry->prev;
        }
        
        dentry_cache_count--;
        spin_unlock(&dentry_cache_lock);
        
        if (dentry->vnode) {
            vnode_put(dentry->vnode);
        }
        
        kfree(dentry);
    } else {
        spin_unlock(&dentry_cache_lock);
    }
}

void dentry_prune(void) {
    /* Remove oldest unused entries */
    dentry_t **p = &dentry_cache;
    while (*p && dentry_cache_count > MAX_DENTRY_CACHE / 2) {
        dentry_t *d = *p;
        if (d->refcount <= 0) {
            *p = d->next;
            if (d->next) {
                d->next->prev = NULL;
            }
            dentry_cache_count--;
            kfree(d);
        } else {
            p = &(*p)->next;
        }
    }
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

void vfs_print_stats(void) {
    console_print("\n=== VFS Statistics ===\n");
    console_print("Path lookups: %lu\n", vfs_lookups);
    console_print("Cache hits: %lu (%.1f%%)\n", 
                 vfs_cache_hits,
                 vfs_lookups > 0 ? 
                 (vfs_cache_hits * 100.0 / vfs_lookups) : 0.0);
    console_print("File operations: %lu\n", vfs_operations);
    console_print("Mounts: %d\n", mount_count);
    console_print("Dentry cache: %d entries\n", dentry_cache_count);
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int vfs_init(void) {
    console_print("Initializing VFS...\n");
    
    /* Initialize locks */
    spin_lock_init(&mount_lock);
    spin_lock_init(&dentry_cache_lock);
    
    /* Initialize dentry cache */
    dentry_cache_init();
    
    /* Create root vnode */
    root_vnode = (vnode_t *)kmalloc(sizeof(vnode_t));
    if (!root_vnode) {
        return -1;
    }
    
    memset(root_vnode, 0, sizeof(vnode_t));
    root_vnode->ino = 1;
    root_vnode->mode = VFS_TYPE_DIR | VFS_MODE_RWX | VFS_MODE_RGRP | VFS_MODE_ROTH;
    root_vnode->nlink = 2;
    root_vnode->refcount = 1;
    
    /* Create root dentry */
    root_dentry = (dentry_t *)kmalloc(sizeof(dentry_t));
    if (!root_dentry) {
        kfree(root_vnode);
        return -1;
    }
    
    memset(root_dentry, 0, sizeof(dentry_t));
    strcpy(root_dentry->name, "/");
    root_dentry->vnode = root_vnode;
    root_dentry->refcount = 1;
    
    /* Add to dentry cache */
    dentry_cache = root_dentry;
    dentry_cache_count = 1;
    
    console_print("vfs: VFS initialized\n");
    console_print("vfs: Root vnode created (inode 1)\n");
    console_print("vfs: Dentry cache initialized (%d entries)\n", MAX_DENTRY_CACHE);
    
    return 0;
}

/*============================================================================
 * DIRECTORY OPERATIONS (STUBS)
 *============================================================================*/

int vfs_mkdir(const char *path, uint16_t mode) {
    (void)path;
    (void)mode;
    vfs_operations++;
    return -1;  /* Not implemented */
}

int vfs_rmdir(const char *path) {
    (void)path;
    vfs_operations++;
    return -1;  /* Not implemented */
}

int vfs_create(const char *path, uint16_t mode) {
    (void)path;
    (void)mode;
    vfs_operations++;
    return -1;  /* Not implemented */
}

int vfs_unlink(const char *path) {
    (void)path;
    vfs_operations++;
    return -1;  /* Not implemented */
}

int vfs_rename(const char *old_path, const char *new_path) {
    (void)old_path;
    (void)new_path;
    vfs_operations++;
    return -1;  /* Not implemented */
}

int vfs_stat(vnode_t *vnode, void *stat_buf) {
    (void)vnode;
    (void)stat_buf;
    return -1;  /* Not implemented */
}

int vfs_fstat(file_t *file, void *stat_buf) {
    (void)file;
    (void)stat_buf;
    return -1;  /* Not implemented */
}
