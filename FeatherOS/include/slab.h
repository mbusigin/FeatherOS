/* FeatherOS - Slab Allocator
 * Sprint 7: Slab Allocator
 *
 * Features:
 * - Object caches for kernel subsystems
 * - Per-CPU caches for lock-free allocation
 * - Partial/full/empty slab states
 * - Cache shrinking and reaping
 * - Pre-allocated caches for common objects
 */

#ifndef FEATHEROS_SLAB_H
#define FEATHEROS_SLAB_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================
 * SLAB CONFIGURATION
 *============================================================================*/

#define SLAB_PAGE_SIZE 4096
#define SLAB_MIN_OBJ_SIZE 8
#define SLAB_MAX_OBJ_SIZE (SLAB_PAGE_SIZE / 2)  /* Max 2KB objects */
#define SLAB_DEFAULT_CACHE_SIZE 64  /* Objects per slab */
#define SLAB_MAX_CACHES 32

/*============================================================================
 * SLAB CACHE FLAGS
 *============================================================================*/

#define SLAB_NONE          0x00
#define SLAB_HW_CACHE_ALIGN 0x01  /* Align objects to cache line */
#define SLAB_RED_ZONE       0x02  /* Red zone around objects */
#define SLAB_POISON         0x04  /* Poison objects on free */
#define SLAB_FREE_POISON    0x08   /* Poison with pattern on free */

/*============================================================================
 * OBJECT SIZES (Power of 2)
 *============================================================================*/

#define SLAB_SIZES {"8", "16", "32", "64", "128", "256", "512", "1024", "2048", "4096"}

/*============================================================================
 * SLAB OBJECT NODE (Freelist)
 *============================================================================*/

typedef struct slab_obj {
    struct slab_obj *next;
} slab_obj_t;

/*============================================================================
 * SLAB STATES
 *============================================================================*/

typedef enum {
    SLAB_EMPTY = 0,    /* No objects allocated */
    SLAB_PARTIAL = 1,  /* Some objects allocated */
    SLAB_FULL = 2      /* All objects allocated */
} slab_state_t;

/*============================================================================
 * SLAB STRUCTURE
 *============================================================================*/

typedef struct slab {
    void *page;              /* Physical page for this slab */
    uint32_t obj_size;       /* Size of each object */
    uint32_t num_objs;       /* Number of objects in this slab */
    uint32_t num_free;       /* Number of free objects */
    slab_obj_t *free_list;   /* List of free objects */
    struct slab *next;       /* Next slab in cache */
    struct slab_cache *cache; /* Parent cache */
} slab_t;

/*============================================================================
 * SLAB CACHE STRUCTURE
 *============================================================================*/

typedef struct slab_cache {
    const char *name;        /* Cache name */
    uint32_t obj_size;       /* Size of objects */
    uint32_t num_objs;      /* Objects per slab */
    uint32_t page_size;      /* Page size (usually 4096) */
    uint32_t align;          /* Alignment requirement */
    uint32_t flags;         /* Cache flags */
    
    /* Slab lists */
    slab_t *slabs_partial;  /* Partially filled slabs */
    slab_t *slabs_full;     /* Fully filled slabs */
    slab_t *slabs_empty;    /* Empty slabs */
    
    /* Statistics */
    uint32_t num_slabs;
    uint32_t num_objs_total;
    uint32_t num_objs_free;
    uint32_t num_objs_alloc;
    uint32_t num_frees;
    uint32_t cache_hits;
    uint32_t cache_misses;
    
    /* Constructor/Destructor */
    void (*ctor)(void *);
    void (*dtor)(void *);
    
    /* Internal state */
    bool initialized;
    uint32_t id;
} slab_cache_t;

/*============================================================================
 * PER-CPU CACHE STRUCTURE
 *============================================================================*/

typedef struct slab_cpu_cache {
    slab_cache_t *cache;     /* Parent cache */
    slab_obj_t *free_list;  /* Per-CPU free list */
    uint32_t free_count;    /* Objects in per-CPU cache */
    uint32_t batch_count;   /* Batch size for refilling */
    uint32_t cpu_id;        /* CPU this cache belongs to */
} slab_cpu_cache_t;

/*============================================================================
 * SLAB STATISTICS
 *============================================================================*/

typedef struct slab_stats {
    uint32_t total_caches;
    uint32_t total_slabs;
    uint32_t total_objs;
    uint32_t total_free;
    uint32_t total_alloc;
    uint32_t total_frees;
    uint64_t cache_hits;
    uint64_t cache_misses;
    uint64_t total_alloc_bytes;
    uint64_t total_free_bytes;
} slab_stats_t;

/*============================================================================
 * SLAB CACHE INITIALIZATION
 *============================================================================*/

/* Initialize slab allocator */
int slab_init(void);

/* Create a new cache */
slab_cache_t *slab_cache_create(const char *name, size_t obj_size, 
                                  size_t align, uint32_t flags,
                                  void (*ctor)(void*), void (*dtor)(void*));

/* Destroy a cache */
void slab_cache_destroy(slab_cache_t *cache);

/* Get or create cache for specific size */
slab_cache_t *slab_cache_lookup(size_t size);

/*============================================================================
 * OBJECT ALLOCATION/FREEING
 *============================================================================*/

/* Allocate object from cache */
void *slab_alloc(slab_cache_t *cache);

/* Free object to cache */
void slab_free(slab_cache_t *cache, void *obj);

/* Allocate from size-based cache (for kmalloc) */
void *slab_alloc_size(size_t size);

/* Free to size-based cache */
void slab_free_size(size_t size, void *obj);

/*============================================================================
 * CACHE MANAGEMENT
 *============================================================================*/

/* Refill per-CPU cache from slab */
void slab_refill_cpu(slab_cpu_cache_t *cpu_cache);

/* Flush per-CPU cache */
void slab_flush_cpu(slab_cpu_cache_t *cpu_cache);

/* Shrink cache (return slabs to buddy allocator) */
uint32_t slab_shrink(slab_cache_t *cache);

/* Reap empty slabs */
uint32_t slab_reap(void);

/*============================================================================
 * STATISTICS
 *============================================================================*/

/* Get cache statistics */
slab_stats_t *slab_get_stats(void);

/* Print cache info */
void slab_cache_info(slab_cache_t *cache);

/* Print all cache statistics */
void slab_print_stats(void);

/* Calculate cache hit rate */
double slab_cache_hit_rate(slab_cache_t *cache);

/*============================================================================
 * PRE-DEFINED CACHES
 *============================================================================*/

/* Initialize standard kernel caches */
void slab_init_caches(void);

/* Common cache names */
#define SLAB_CACHE_TASK    "task_struct"
#define SLAB_CACHE_VNODE   "vnode"
#define SLAB_CACHE_FILE    "file_descriptor"
#define SLAB_CACHE_BUFFER  "buffer"
#define SLAB_CACHE_SKB     "sk_buff"
#define SLAB_CACHE_DENTRY  "dentry"
#define SLAB_CACHE_INODE   "inode"

/*============================================================================
 * KMALLOC INTEGRATION
 *============================================================================*/

/* kmalloc using slab allocator */
void *slab_kmalloc(size_t size);

/* kfree using slab allocator */
void slab_kfree(void *ptr, size_t size);

/*============================================================================
 * CACHE DEBUG
 *============================================================================*/

#ifdef DEBUG
void slab_cache_validate(slab_cache_t *cache);
void slab_dump_cache(slab_cache_t *cache);
#else
#define slab_cache_validate(c) ((void)0)
#define slab_dump_cache(c) ((void)0)
#endif

#endif /* FEATHEROS_SLAB_H */
