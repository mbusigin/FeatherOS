/* FeatherOS - Physical Memory Manager
 * Sprint 5: Physical Memory Management
 *
 * Features:
 * - E820 memory map detection
 * - Bitmap-based physical memory allocator
 * - Memory regions (free, reserved, ACPI, kernel)
 * - Memory statistics
 * - Memory test utility
 */

#ifndef FEATHEROS_MEMORY_H
#define FEATHEROS_MEMORY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <paging.h>

/* Memory constants */
#define MEMORY_BITMAP_MAGIC 0xFEAD

/* Memory region types */
typedef enum {
    MEMORY_REGION_FREE = 1,
    MEMORY_REGION_RESERVED = 2,
    MEMORY_REGION_ACPI = 3,
    MEMORY_REGION_NVS = 4,
    MEMORY_REGION_KERNEL = 5,
    MEMORY_REGION_BOOTLOADER = 6,
    MEMORY_REGION_HOLE = 7
} memory_region_type_t;

/* Memory region descriptor */
typedef struct memory_region {
    uint64_t base;               /* Base address */
    uint64_t length;             /* Length in bytes */
    memory_region_type_t type;    /* Region type */
    const char *name;           /* Optional name */
} memory_region_t;

/* Memory statistics */
typedef struct memory_stats {
    uint64_t total_pages;        /* Total pages in system */
    uint64_t free_pages;         /* Free pages */
    uint64_t reserved_pages;      /* Reserved pages */
    uint64_t kernel_pages;       /* Kernel pages */
    uint64_t largest_free;       /* Largest free region in pages */
    uint64_t allocated_pages;     /* Pages allocated */
    uint64_t freed_pages;        /* Pages freed */
} memory_stats_t;

/* Memory manager state */
typedef struct memory_manager {
    uint64_t total_memory;        /* Total detected memory in bytes */
    uint64_t bitmap_phys_addr;   /* Physical address of bitmap */
    uint32_t *bitmap;            /* Pointer to bitmap */
    size_t bitmap_size;           /* Bitmap size in bytes */
    uint64_t bitmap_pages;       /* Pages needed for bitmap */
    uint64_t memory_start;        /* Start of managed memory */
    uint64_t memory_end;          /* End of managed memory */
    uint64_t max_pages;          /* Maximum physical pages */
    memory_stats_t stats;         /* Statistics */
    uint32_t magic;              /* Magic number for validation */
} memory_manager_t;

/* Global memory manager */
extern memory_manager_t *mem_manager;

/*============================================================================
 * E820 MEMORY DETECTION
 *============================================================================*/

/* E820 map entry types */
#define E820_TYPE_FREE 1
#define E820_TYPE_RESERVED 2
#define E820_TYPE_ACPI 3
#define E820_TYPE_NVS 4

/* E820 memory map entry */
typedef struct e820_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_ext;
} e820_entry_t;

/* Maximum E820 entries */
#define E820_MAX_ENTRIES 128
#define E820_SIGNATURE 0x0000E820
#define E820_SMAP 0x534D4150

/*============================================================================
 * MEMORY MANAGER INITIALIZATION
 *============================================================================*/

/* Initialize memory manager */
int memory_manager_init(memory_manager_t *mm);

/* Free memory manager resources */
void memory_manager_destroy(memory_manager_t *mm);

/* Get memory manager singleton */
memory_manager_t *get_memory_manager(void);

/*============================================================================
 * PHYSICAL PAGE ALLOCATION
 *============================================================================*/

/* Allocate a physical page */
uint64_t physical_alloc_page(void);

/* Free a physical page */
void physical_free_page(uint64_t addr);

/* Allocate multiple consecutive pages */
uint64_t physical_alloc_pages(size_t count);

/* Free multiple consecutive pages */
void physical_free_pages(uint64_t addr, size_t count);

/* Check if a page is allocated */
bool physical_is_allocated(uint64_t addr);

/* Reserve a page range (mark as used but don't track as allocated) */
void physical_reserve(uint64_t addr, size_t count);

/* Unreserve a page range */
void physical_unreserve(uint64_t addr, size_t count);

/* Get statistics */
memory_stats_t *memory_get_stats(void);

/* Print memory map */
void memory_print_map(void);

/* Print memory statistics */
void memory_print_stats(void);

/*============================================================================
 * MEMORY TEST UTILITY
 *============================================================================*/

/* Test memory for errors */
typedef struct memory_test_result {
    bool passed;
    uint64_t pages_tested;
    uint64_t errors_found;
    uint64_t first_error_addr;
    uint64_t first_error_expected;
    uint64_t first_error_actual;
} memory_test_result_t;

/* Run memory test */
memory_test_result_t memory_test(void);

/* Quick memory test (less thorough) */
bool memory_quick_test(void);

/*============================================================================
 * MEMORY UTILITIES
 *============================================================================*/

/* Convert bytes to pages */
static inline size_t bytes_to_pages(uint64_t bytes) {
    return (bytes + PAGE_SIZE - 1) >> PAGE_SHIFT;
}

/* Convert pages to bytes */
static inline uint64_t pages_to_bytes(size_t pages) {
    return (uint64_t)pages << PAGE_SHIFT;
}

/* Align address up to next page */
static inline uint64_t page_align_up(uint64_t addr) {
    return (addr + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
}

/* Align address down to page boundary */
static inline uint64_t page_align_down(uint64_t addr) {
    return addr & ~(PAGE_SIZE - 1);
}

/* Check if address is page aligned */
static inline bool is_page_aligned(uint64_t addr) {
    return (addr & (PAGE_SIZE - 1)) == 0;
}

/* Get page number from address */
static inline size_t addr_to_page(uint64_t addr) {
    return addr >> PAGE_SHIFT;
}

/* Get address from page number */
static inline uint64_t page_to_addr(size_t page) {
    return (uint64_t)page << PAGE_SHIFT;
}

#endif /* FEATHEROS_MEMORY_H */
