/* FeatherOS - Slab Allocator Implementation
 * Sprint 7: Slab Allocator
 */

#include <slab.h>
#include <paging.h>
#include <memory.h>
#include <kernel.h>
#include <datastructures.h>
#include <string.h>

/*============================================================================
 * GLOBALS
 *============================================================================*/

/* Per-CPU cache array */
static slab_cpu_cache_t per_cpu_caches[16] __attribute__((aligned(64)));

/* Size-based caches (power of 2) */
static slab_cache_t *size_caches[10] = {NULL};
static uint32_t size_cache_sizes[10] = {8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096};

/* Global statistics */
static slab_stats_t global_stats = {0};
static uint32_t cache_id_counter = 0;
static bool slab_initialized = false;

/*============================================================================
 * HELPERS
 *============================================================================*/

static void *slab_alloc_page(void) {
    return (void*)(uintptr_t)physical_alloc_page();
}

static void slab_free_page(void *page) {
    physical_free_page((uint64_t)(uintptr_t)page);
}

static uint32_t calc_num_objs(uint32_t obj_size, uint32_t page_size, uint32_t align) {
    (void)align;
    uint32_t overhead = sizeof(slab_t);
    uint32_t usable = page_size - overhead;
    uint32_t num = usable / (obj_size + sizeof(slab_obj_t*));
    return num > 0 ? num : 1;
}

/*============================================================================
 * SLAB CACHE CREATION
 *============================================================================*/

slab_cache_t *slab_cache_create(const char *name, size_t obj_size,
                                  size_t align, uint32_t flags,
                                  void (*ctor)(void*), void (*dtor)(void*)) {
    slab_cache_t *cache = kmalloc(sizeof(slab_cache_t));
    if (!cache) return NULL;
    
    memset(cache, 0, sizeof(slab_cache_t));
    
    cache->name = name;
    cache->obj_size = (uint32_t)obj_size;
    cache->align = (uint32_t)(align > 0 ? align : 8);
    cache->flags = flags;
    cache->ctor = ctor;
    cache->dtor = dtor;
    cache->page_size = SLAB_PAGE_SIZE;
    cache->num_objs = calc_num_objs(cache->obj_size, cache->page_size, cache->align);
    cache->initialized = true;
    cache->id = cache_id_counter++;
    
    global_stats.total_caches++;
    
    return cache;
}

void slab_cache_destroy(slab_cache_t *cache) {
    if (!cache) return;
    
    /* Free all slabs */
    slab_t *slab = cache->slabs_partial;
    while (slab) {
        slab_t *next = slab->next;
        slab_free_page(slab->page);
        kfree(slab);
        slab = next;
    }
    
    slab = cache->slabs_full;
    while (slab) {
        slab_t *next = slab->next;
        slab_free_page(slab->page);
        kfree(slab);
        slab = next;
    }
    
    slab = cache->slabs_empty;
    while (slab) {
        slab_t *next = slab->next;
        slab_free_page(slab->page);
        kfree(slab);
        slab = next;
    }
    
    kfree(cache);
    global_stats.total_caches--;
}

/*============================================================================
 * SLAB CREATION
 *============================================================================*/

static slab_t *slab_create(slab_cache_t *cache) {
    void *page = slab_alloc_page();
    if (!page) return NULL;
    
    slab_t *slab = (slab_t*)kmalloc(sizeof(slab_t));
    if (!slab) {
        slab_free_page(page);
        return NULL;
    }
    
    memset(slab, 0, sizeof(slab_t));
    slab->page = page;
    slab->obj_size = cache->obj_size;
    slab->num_objs = cache->num_objs;
    slab->num_free = slab->num_objs;
    slab->cache = cache;
    
    /* Build free list */
    uint8_t *obj_ptr = (uint8_t*)page + sizeof(slab_t);
    slab_obj_t **free_ptr = &slab->free_list;
    
    for (uint32_t i = 0; i < slab->num_objs; i++) {
        *free_ptr = (slab_obj_t*)obj_ptr;
        free_ptr = &((*free_ptr)->next);
        obj_ptr += cache->obj_size;
    }
    *free_ptr = NULL;
    
    cache->num_slabs++;
    cache->num_objs_total += slab->num_objs;
    cache->num_objs_free += slab->num_free;
    global_stats.total_slabs++;
    global_stats.total_objs += slab->num_objs;
    global_stats.total_free += slab->num_free;
    
    return slab;
}

/*============================================================================
 * OBJECT ALLOCATION
 *============================================================================*/

void *slab_alloc(slab_cache_t *cache) {
    if (!cache || !cache->initialized) return NULL;
    
    /* Try partial slab first */
    if (cache->slabs_partial) {
        slab_t *slab = cache->slabs_partial;
        slab_obj_t *obj = slab->free_list;
        slab->free_list = obj->next;
        slab->num_free--;
        cache->num_objs_free--;
        cache->num_objs_alloc++;
        cache->cache_hits++;
        global_stats.cache_hits++;
        
        /* Move to full list if needed */
        if (slab->num_free == 0) {
            cache->slabs_partial = slab->next;
            slab->next = cache->slabs_full;
            cache->slabs_full = slab;
        }
        
        if (cache->ctor) cache->ctor(obj);
        return obj;
    }
    
    /* Try empty slab */
    if (cache->slabs_empty) {
        slab_t *slab = cache->slabs_empty;
        cache->slabs_empty = slab->next;
        slab->next = cache->slabs_partial;
        cache->slabs_partial = slab;
        cache->cache_hits++;
        global_stats.cache_hits++;
        return slab_alloc(cache);  /* Retry with partial slab */
    }
    
    /* Create new slab */
    slab_t *slab = slab_create(cache);
    if (!slab) {
        cache->cache_misses++;
        global_stats.cache_misses++;
        return NULL;
    }
    
    slab->next = cache->slabs_partial;
    cache->slabs_partial = slab;
    cache->cache_hits++;
    global_stats.cache_hits++;
    return slab_alloc(cache);  /* Retry */
}

/*============================================================================
 * OBJECT FREEING
 *============================================================================*/

void slab_free(slab_cache_t *cache, void *obj) {
    if (!cache || !obj) return;
    
    /* Find which slab this object belongs to */
    slab_t **slab_ptr = &cache->slabs_full;
    while (*slab_ptr) {
        slab_t *slab = *slab_ptr;
        uint64_t offset = (uint8_t*)obj - (uint8_t*)slab->page;
        if (offset < cache->page_size) {
            /* Found the slab */
            if (cache->dtor) cache->dtor(obj);
            
            slab_obj_t *obj_node = (slab_obj_t*)obj;
            obj_node->next = slab->free_list;
            slab->free_list = obj_node;
            slab->num_free++;
            cache->num_objs_free++;
            cache->num_frees++;
            global_stats.total_frees++;
            
            /* Move slab between lists */
            if (slab->num_free == 1) {
                /* Was full, now partial */
                *slab_ptr = slab->next;
                slab->next = cache->slabs_partial;
                cache->slabs_partial = slab;
            } else if (slab->num_free == slab->num_objs) {
                /* Now empty - could move to empty list */
                if (slab_ptr == &cache->slabs_full) {
                    *slab_ptr = slab->next;
                } else if (slab_ptr == &cache->slabs_partial) {
                    *slab_ptr = slab->next;
                }
                slab->next = cache->slabs_empty;
                cache->slabs_empty = slab;
            }
            return;
        }
        slab_ptr = &((*slab_ptr)->next);
    }
    
    /* Check partial slabs */
    slab_ptr = &cache->slabs_partial;
    while (*slab_ptr) {
        slab_t *slab = *slab_ptr;
        uint64_t offset = (uint8_t*)obj - (uint8_t*)slab->page;
        if (offset < cache->page_size) {
            if (cache->dtor) cache->dtor(obj);
            
            slab_obj_t *obj_node = (slab_obj_t*)obj;
            obj_node->next = slab->free_list;
            slab->free_list = obj_node;
            slab->num_free++;
            cache->num_objs_free++;
            cache->num_frees++;
            global_stats.total_frees++;
            
            if (slab->num_free == slab->num_objs) {
                *slab_ptr = slab->next;
                slab->next = cache->slabs_empty;
                cache->slabs_empty = slab;
            }
            return;
        }
        slab_ptr = &((*slab_ptr)->next);
    }
}

/*============================================================================
 * SIZE-BASED ALLOCATION
 *============================================================================*/

static void make_cache_name(char *buf, uint32_t size) {
    (void)buf;
    /* Simple integer to string without snprintf */
    char tmp[16];
    char *p = tmp;
    uint32_t val = size;
    int digits = 0;
    
    if (size == 0) {
        *p++ = '0';
    } else {
        while (val > 0) {
            val /= 10;
            digits++;
        }
        val = size;
        for (int i = digits - 1; i >= 0; i--) {
            p[i] = '0' + (val % 10);
            val /= 10;
        }
        p += digits;
    }
    *p = '\0';
    
    /* Copy to buf with "size-" prefix */
    buf[0] = 's'; buf[1] = 'i'; buf[2] = 'z'; buf[3] = 'e'; buf[4] = '-';
    for (int i = 0; tmp[i]; i++) {
        buf[5 + i] = tmp[i];
    }
}

slab_cache_t *slab_cache_lookup(size_t size) {
    if (size == 0) return NULL;
    
    /* Find appropriate cache */
    for (int i = 0; i < 10; i++) {
        if (size <= size_cache_sizes[i]) {
            if (!size_caches[i]) {
                char name[32];
                make_cache_name(name, size_cache_sizes[i]);
                size_caches[i] = slab_cache_create(name, size_cache_sizes[i], 8, 0, NULL, NULL);
            }
            return size_caches[i];
        }
    }
    
    /* Too large for slab, use page allocator */
    return NULL;
}

void *slab_alloc_size(size_t size) {
    slab_cache_t *cache = slab_cache_lookup(size);
    if (!cache) {
        /* Allocate full page for large objects */
        return (void*)(uintptr_t)physical_alloc_page();
    }
    return slab_alloc(cache);
}

void slab_free_size(size_t size, void *obj) {
    if (!obj) return;
    
    slab_cache_t *cache = slab_cache_lookup(size);
    if (cache) {
        slab_free(cache, obj);
    } else {
        /* Free page for large objects */
        physical_free_page((uint64_t)(uintptr_t)obj);
    }
}

/*============================================================================
 * KMALLOC INTEGRATION
 *============================================================================*/

void *slab_kmalloc(size_t size) {
    if (size == 0) return NULL;
    return slab_alloc_size(size);
}

void slab_kfree(void *ptr, size_t size) {
    if (!ptr) return;
    slab_free_size(size, ptr);
}

/*============================================================================
 * CACHE MANAGEMENT
 *============================================================================*/

void slab_refill_cpu(slab_cpu_cache_t *cpu_cache) {
    (void)cpu_cache;
    /* Per-CPU caching is complex - simplified for now */
}

void slab_flush_cpu(slab_cpu_cache_t *cpu_cache) {
    (void)cpu_cache;
}

uint32_t slab_shrink(slab_cache_t *cache) {
    if (!cache) return 0;
    
    uint32_t freed_slabs = 0;
    while (cache->slabs_empty && freed_slabs < 4) {
        slab_t *slab = cache->slabs_empty;
        cache->slabs_empty = slab->next;
        slab_free_page(slab->page);
        kfree(slab);
        cache->num_slabs--;
        cache->num_objs_total -= slab->num_objs;
        cache->num_objs_free -= slab->num_free;
        global_stats.total_slabs--;
        global_stats.total_objs -= slab->num_objs;
        global_stats.total_free -= slab->num_free;
        freed_slabs++;
    }
    return freed_slabs;
}

uint32_t slab_reap(void) {
    uint32_t total = 0;
    for (int i = 0; i < 10; i++) {
        if (size_caches[i]) {
            total += slab_shrink(size_caches[i]);
        }
    }
    return total;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

slab_stats_t *slab_get_stats(void) {
    global_stats.total_free = 0;
    for (int i = 0; i < 10; i++) {
        if (size_caches[i]) {
            global_stats.total_free += size_caches[i]->num_objs_free;
        }
    }
    return &global_stats;
}

double slab_cache_hit_rate(slab_cache_t *cache) {
    if (!cache) return 0.0;
    uint64_t total = cache->cache_hits + cache->cache_misses;
    if (total == 0) return 0.0;
    return (double)cache->cache_hits / (double)total * 100.0;
}

void slab_cache_info(slab_cache_t *cache) {
    if (!cache) return;
    console_print("Cache: %s\n", cache->name);
    console_print("  Object size: %u\n", cache->obj_size);
    console_print("  Objects/slab: %u\n", cache->num_objs);
    console_print("  Slabs: %u\n", cache->num_slabs);
    console_print("  Free: %u/%u\n", cache->num_objs_free, cache->num_objs_total);
    console_print("  Hit rate: %.1f%%\n", slab_cache_hit_rate(cache));
}

void slab_print_stats(void) {
    slab_stats_t *stats = slab_get_stats();
    console_print("Slab Allocator Statistics:\n");
    console_print("  Caches: %u\n", stats->total_caches);
    console_print("  Slabs: %u\n", stats->total_slabs);
    console_print("  Total objects: %u\n", stats->total_objs);
    console_print("  Free objects: %u\n", stats->total_free);
    console_print("  Cache hits: %lu\n", stats->cache_hits);
    console_print("  Cache misses: %lu\n", stats->cache_misses);
    
    if (stats->cache_hits + stats->cache_misses > 0) {
        double rate = (double)stats->cache_hits / 
                     (double)(stats->cache_hits + stats->cache_misses) * 100.0;
        console_print("  Overall hit rate: %.1f%%\n", rate);
    }
    console_print("\n");
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int slab_init(void) {
    if (slab_initialized) return 0;
    
    /* Initialize per-CPU caches */
    for (int i = 0; i < 16; i++) {
        per_cpu_caches[i].cpu_id = i;
        per_cpu_caches[i].batch_count = 8;
    }
    
    /* Pre-create size caches */
    for (int i = 0; i < 10; i++) {
        char name[32];
        make_cache_name(name, size_cache_sizes[i]);
        size_caches[i] = slab_cache_create(name, size_cache_sizes[i], 8, 0, NULL, NULL);
    }
    
    slab_initialized = true;
    return 0;
}

void slab_init_caches(void) {
    slab_init();
}
