/* FeatherOS - Physical Memory Manager Implementation
 * Sprint 5: Physical Memory Management
 */

#include <memory.h>
#include <kernel.h>
#include <string.h>

/* Global memory manager */
memory_manager_t *mem_manager = NULL;

/* Bitmap operations */
#define BITMAP_SET(bitmap, bit) ((bitmap)[(bit) / 32] |= (1U << ((bit) % 32)))
#define BITMAP_CLR(bitmap, bit) ((bitmap)[(bit) / 32] &= ~(1U << ((bit) % 32)))
#define BITMAP_TEST(bitmap, bit) (((bitmap)[(bit) / 32] & (1U << ((bit) % 32))) != 0)

/* Find first zero bit in a bitmap word (reserved for future use) */
static int find_first_zero(uint32_t word) __attribute__((unused));
static int find_first_zero(uint32_t word) {
    for (int i = 0; i < 32; i++) {
        if ((word & (1U << i)) == 0) return i;
    }
    return -1;
}

/* Find first free contiguous bits */
static int find_first_free(uint32_t *bitmap, size_t max_bits, size_t count) {
    for (size_t i = 0; i < max_bits; i++) {
        bool found = true;
        for (size_t j = 0; j < count; j++) {
            if (BITMAP_TEST(bitmap, i + j)) {
                found = false;
                break;
            }
        }
        if (found) return i;
    }
    return -1;
}

/*============================================================================
 * E820 MEMORY DETECTION
 *============================================================================*/

int memory_detect_e820(e820_entry_t *entries, int max_entries) {
    /* This would normally call BIOS INT 15h AX=E820
     * For now, we'll use simulated values */
    memset(entries, 0, sizeof(e820_entry_t) * max_entries);
    
    /* Simulated E820 map for QEMU (256MB RAM) */
    entries[0].base = 0x00000000;
    entries[0].length = 0x9F000;
    entries[0].type = E820_TYPE_FREE;
    
    entries[1].base = 0x0009F000;
    entries[1].length = 0x61000;
    entries[1].type = E820_TYPE_RESERVED;
    
    entries[2].base = 0x00100000;
    entries[2].length = 0x0FF00000;  /* 255MB free */
    entries[2].type = E820_TYPE_FREE;
    
    entries[3].base = 0x100000000;
    entries[3].length = 0x10000000;  /* 256MB more at 4GB */
    entries[3].type = E820_TYPE_FREE;
    
    return 4;
}

/*============================================================================
 * MEMORY MANAGER INITIALIZATION
 *============================================================================*/

int memory_manager_init(memory_manager_t *mm) {
    memset(mm, 0, sizeof(memory_manager_t));
    
    /* Detect memory via E820 */
    e820_entry_t entries[E820_MAX_ENTRIES];
    int count = memory_detect_e820(entries, E820_MAX_ENTRIES);
    
    /* Find total usable memory */
    uint64_t total = 0;
    for (int i = 0; i < count; i++) {
        if (entries[i].type == E820_TYPE_FREE) {
            total += entries[i].length;
        }
    }
    
    mm->total_memory = total;
    mm->max_pages = bytes_to_pages(total);
    
    /* Calculate bitmap size (1 bit per page) */
    mm->bitmap_pages = bytes_to_pages((mm->max_pages + 7) / 8);
    mm->bitmap_size = mm->bitmap_pages * PAGE_SIZE;
    
    /* Reserve memory for bitmap (put it after kernel) */
    mm->bitmap_phys_addr = 0x200000;  /* 2MB */
    mm->bitmap = (uint32_t*)(uintptr_t)mm->bitmap_phys_addr;
    
    /* Clear bitmap */
    memset(mm->bitmap, 0xFF, mm->bitmap_size);  /* All allocated initially */
    
    /* Mark free pages */
    for (int i = 0; i < count; i++) {
        if (entries[i].type == E820_TYPE_FREE) {
            size_t start_page = bytes_to_pages(entries[i].base);
            size_t num_pages = bytes_to_pages(entries[i].length);
            for (size_t p = 0; p < num_pages && (start_page + p) < mm->max_pages; p++) {
                if (start_page + p >= bytes_to_pages(mm->bitmap_phys_addr) &&
                    start_page + p < bytes_to_pages(mm->bitmap_phys_addr) + mm->bitmap_pages) {
                    /* Skip pages used by bitmap itself */
                    continue;
                }
                BITMAP_CLR(mm->bitmap, start_page + p);
            }
        }
    }
    
    /* Initialize statistics */
    mm->stats.total_pages = mm->max_pages;
    mm->stats.free_pages = mm->max_pages;
    mm->stats.reserved_pages = mm->bitmap_pages;
    mm->stats.kernel_pages = 0;
    mm->stats.largest_free = mm->stats.free_pages;
    mm->stats.allocated_pages = 0;
    mm->stats.freed_pages = 0;
    
    mm->magic = MEMORY_BITMAP_MAGIC;
    mem_manager = mm;
    
    return 0;
}

void memory_manager_destroy(memory_manager_t *mm) {
    UNUSED(mm);
    mem_manager = NULL;
}

memory_manager_t *get_memory_manager(void) {
    return mem_manager;
}

/*============================================================================
 * PHYSICAL PAGE ALLOCATION
 *============================================================================*/

uint64_t physical_alloc_page(void) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return 0;
    }
    
    int page = find_first_free(mm->bitmap, mm->max_pages, 1);
    if (page < 0) {
        return 0;  /* No free pages */
    }
    
    BITMAP_SET(mm->bitmap, page);
    mm->stats.free_pages--;
    mm->stats.allocated_pages++;
    
    if (mm->stats.free_pages < mm->stats.largest_free) {
        mm->stats.largest_free = mm->stats.free_pages;
    }
    
    return page_to_addr(page);
}

void physical_free_page(uint64_t addr) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return;
    }
    
    size_t page = addr_to_page(addr);
    if (page >= mm->max_pages) {
        return;
    }
    
    if (BITMAP_TEST(mm->bitmap, page)) {
        BITMAP_CLR(mm->bitmap, page);
        mm->stats.free_pages++;
        mm->stats.freed_pages++;
    }
}

uint64_t physical_alloc_pages(size_t count) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return 0;
    }
    
    int page = find_first_free(mm->bitmap, mm->max_pages, count);
    if (page < 0) {
        return 0;  /* No free pages */
    }
    
    for (size_t i = 0; i < count; i++) {
        BITMAP_SET(mm->bitmap, page + i);
        mm->stats.free_pages--;
    }
    mm->stats.allocated_pages += count;
    
    if (mm->stats.free_pages < mm->stats.largest_free) {
        mm->stats.largest_free = mm->stats.free_pages;
    }
    
    return page_to_addr(page);
}

void physical_free_pages(uint64_t addr, size_t count) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return;
    }
    
    size_t start_page = addr_to_page(addr);
    for (size_t i = 0; i < count; i++) {
        if (start_page + i < mm->max_pages && BITMAP_TEST(mm->bitmap, start_page + i)) {
            BITMAP_CLR(mm->bitmap, start_page + i);
            mm->stats.free_pages++;
            mm->stats.freed_pages++;
        }
    }
}

bool physical_is_allocated(uint64_t addr) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return true;
    }
    
    size_t page = addr_to_page(addr);
    if (page >= mm->max_pages) {
        return true;
    }
    
    return BITMAP_TEST(mm->bitmap, page);
}

void physical_reserve(uint64_t addr, size_t count) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return;
    }
    
    size_t start_page = addr_to_page(addr);
    for (size_t i = 0; i < count; i++) {
        if (start_page + i < mm->max_pages) {
            if (!BITMAP_TEST(mm->bitmap, start_page + i)) {
                BITMAP_SET(mm->bitmap, start_page + i);
                mm->stats.free_pages--;
            }
            mm->stats.reserved_pages++;
        }
    }
}

void physical_unreserve(uint64_t addr, size_t count) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        return;
    }
    
    size_t start_page = addr_to_page(addr);
    for (size_t i = 0; i < count; i++) {
        if (start_page + i < mm->max_pages) {
            if (!BITMAP_TEST(mm->bitmap, start_page + i)) {
                BITMAP_SET(mm->bitmap, start_page + i);
                mm->stats.free_pages++;
            }
            mm->stats.reserved_pages--;
        }
    }
}

memory_stats_t *memory_get_stats(void) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm) return NULL;
    return &mm->stats;
}

/*============================================================================
 * MEMORY TEST UTILITY
 *============================================================================*/

memory_test_result_t memory_test(void) {
    memory_test_result_t result = {0};
    memory_manager_t *mm = get_memory_manager();
    
    if (!mm || mm->magic != MEMORY_BITMAP_MAGIC) {
        result.passed = false;
        return result;
    }
    
    result.pages_tested = 0;
    result.errors_found = 0;
    
    /* Test free pages with patterns */
    for (size_t i = 0; i < mm->max_pages && result.pages_tested < 1000; i++) {
        if (!BITMAP_TEST(mm->bitmap, i)) {
            uint64_t addr = page_to_addr(i);
            volatile uint64_t *ptr = (volatile uint64_t*)(uintptr_t)addr;
            
            /* Test 1: Write and read 0xAA55AA55 */
            *ptr = 0xAA55AA55;
            if (*ptr != 0xAA55AA55) {
                result.errors_found++;
                if (result.errors_found == 1) {
                    result.first_error_addr = addr;
                    result.first_error_expected = 0xAA55AA55;
                    result.first_error_actual = *ptr;
                }
            }
            result.pages_tested++;
            
            /* Test 2: Write and read 0x55AA55AA */
            *ptr = 0x55AA55AA;
            if (*ptr != 0x55AA55AA) {
                result.errors_found++;
                if (result.errors_found == 1) {
                    result.first_error_addr = addr;
                    result.first_error_expected = 0x55AA55AA;
                    result.first_error_actual = *ptr;
                }
            }
            result.pages_tested++;
            
            /* Test 3: Write and read 0xFFFFFFFF */
            *ptr = 0xFFFFFFFF;
            if (*ptr != 0xFFFFFFFF) {
                result.errors_found++;
            }
            result.pages_tested++;
            
            /* Test 4: Write and read 0x00000000 */
            *ptr = 0x00000000;
            if (*ptr != 0x00000000) {
                result.errors_found++;
            }
            result.pages_tested++;
        }
    }
    
    result.passed = (result.errors_found == 0);
    return result;
}

bool memory_quick_test(void) {
    memory_test_result_t result = memory_test();
    return result.passed;
}

/*============================================================================
 * PRINT FUNCTIONS
 *============================================================================*/

void memory_print_map(void) {
    memory_manager_t *mm = get_memory_manager();
    if (!mm) {
        console_print("Memory manager not initialized\n");
        return;
    }
    
    console_print("Memory Map:\n");
    console_print("  Total Memory: %lu MB\n", mm->total_memory / (1024 * 1024));
    console_print("  Total Pages: %lu\n", mm->max_pages);
    console_print("  Bitmap at: 0x%lx\n", mm->bitmap_phys_addr);
    console_print("  Bitmap size: %lu bytes\n", mm->bitmap_size);
    console_print("\n");
}

void memory_print_stats(void) {
    memory_stats_t *stats = memory_get_stats();
    if (!stats) {
        console_print("Memory statistics not available\n");
        return;
    }
    
    console_print("Memory Statistics:\n");
    console_print("  Total Pages: %lu\n", stats->total_pages);
    console_print("  Free Pages: %lu (%.1f MB)\n", 
        stats->free_pages, 
        (double)(stats->free_pages * PAGE_SIZE) / (1024 * 1024));
    console_print("  Allocated Pages: %lu\n", stats->allocated_pages);
    console_print("  Freed Pages: %lu\n", stats->freed_pages);
    console_print("  Reserved Pages: %lu\n", stats->reserved_pages);
    console_print("  Kernel Pages: %lu\n", stats->kernel_pages);
    console_print("  Largest Free Region: %lu pages (%.1f MB)\n",
        stats->largest_free,
        (double)(stats->largest_free * PAGE_SIZE) / (1024 * 1024));
    console_print("\n");
}
