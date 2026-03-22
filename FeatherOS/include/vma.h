/* FeatherOS - Virtual Memory Areas (VMA)
 * Sprint 8: Virtual Memory Areas
 */

#ifndef FEATHEROS_VMA_H
#define FEATHEROS_VMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================
 * VMA FLAGS
 *============================================================================*/

#define VMA_READ       0x01  /* Read permission */
#define VMA_WRITE      0x02  /* Write permission */
#define VMA_EXEC       0x04  /* Execute permission */
#define VMA_PRIVATE    0x10  /* Private mapping */
#define VMA_SHARED     0x20  /* Shared mapping */
#define VMA_ANONYMOUS  0x40  /* Anonymous (no backing file) */
#define VMA_FIXED      0x80  /* Fixed address hint */
#define VMA_NO_MERGE   0x100 /* Don't merge with adjacent */
#define VMA_GROWSDOWN  0x200 /* Stack grows down */
#define VMA_GROWSUP    0x400 /* Stack grows up */
#define VMA_STACK      0x800 /* Stack VMA */
#define VMA_HEAP       0x1000 /* Heap VMA */

/*============================================================================
 * VMA STRUCTURE
 *============================================================================*/

typedef struct vma {
    uint64_t start;          /* Start address */
    uint64_t end;            /* End address */
    uint64_t offset;         /* File offset */
    uint32_t flags;          /* VMA flags */
    uint32_t id;             /* Unique VMA ID */
    uint32_t ref_count;      /* Reference count */
    uint64_t inode;          /* File inode (if file-backed) */
    const char *name;        /* VMA name (for /proc/maps) */
    struct vma *next;        /* Linked list (fallback) */
} vma_t;

/*============================================================================
 * VMA MANAGER
 *============================================================================*/

typedef struct {
    vma_t *vmas;            /* List of VMAs */
    void *vma_tree;         /* RB-tree for fast lookup */
    uint32_t num_vmas;      /* Number of VMAs */
} vma_manager_t;

/*============================================================================
 * VMA STATISTICS
 *============================================================================*/

typedef struct {
    uint32_t total_vmas;
    uint32_t anon_vmas;
    uint32_t file_vmas;
    uint32_t stack_vmas;
    uint32_t heap_vmas;
    uint64_t total_size;
} vma_stats_t;

/*============================================================================
 * VMA MANAGER
 *============================================================================*/

/* Initialize VMA manager */
int vma_manager_init(vma_manager_t *manager);

/* Get global VMA manager */
vma_manager_t *get_vma_manager(void);

/*============================================================================
 * VMA OPERATIONS
 *============================================================================*/

/* Create a new VMA */
vma_t *vma_create(uint64_t start, uint64_t end, uint32_t flags, const char *name);

/* Destroy a VMA */
void vma_destroy(vma_t *vma);

/* Insert VMA into manager - O(log n) via RB-tree */
int vma_insert(vma_manager_t *manager, vma_t *vma);

/* Find VMA containing address - O(log n) via RB-tree */
vma_t *vma_find(vma_manager_t *manager, uint64_t addr);

/* Remove VMA from manager */
int vma_remove(vma_manager_t *manager, vma_t *vma);

/* Merge adjacent VMAs with same flags */
int vma_merge(vma_manager_t *manager);

/*============================================================================
 * MMAP / MUNMAP
 *============================================================================*/

/* Memory map */
void *vma_mmap(vma_manager_t *manager, uint64_t addr, size_t length,
               int prot, int flags, const char *name);

/* Memory unmap */
int vma_munmap(vma_manager_t *manager, uint64_t addr, size_t length);

/* Change protection */
int vma_mprotect(vma_manager_t *manager, uint64_t addr, size_t length, int prot);

/*============================================================================
 * /proc/PID/maps
 *============================================================================*/

/* Print /proc/PID/maps */
void vma_print_maps(vma_manager_t *manager);

/*============================================================================
 * STATISTICS
 *============================================================================*/

/* Get VMA statistics */
vma_stats_t *vma_get_stats(vma_manager_t *manager);

/* Print VMA statistics */
void vma_print_stats(vma_manager_t *manager);

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

/* Initialize VMA subsystem */
void vma_init(void);

#endif /* FEATHEROS_VMA_H */
