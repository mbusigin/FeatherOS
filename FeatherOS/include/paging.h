/* FeatherOS - Virtual Memory / Paging
 * Sprint 6: Virtual Memory (Paging)
 *
 * Features:
 * - 4-level page tables (PML4/PDP/PD/PT)
 * - Page fault handler
 * - Kernel virtual address space
 * - vmalloc/vfree
 * - PTE flags (present/rw/user/nx)
 */

#ifndef FEATHEROS_PAGING_H
#define FEATHEROS_PAGING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/*============================================================================
 * PAGE TABLE STRUCTURE
 *============================================================================*/

/* Page size: 4KB */
#define PAGE_SHIFT      12
#define PAGE_SIZE       (1 << PAGE_SHIFT)       /* 4096 bytes */
#define PAGE_MASK       (PAGE_SIZE - 1)

/* Page table entry bits */
#define PTE_PRESENT     (1 << 0)   /* Present */
#define PTE_WRITABLE    (1 << 1)   /* Read/Write */
#define PTE_USER        (1 << 2)   /* User/Supervisor */
#define PTE_WRITE_THROUGH (1 << 3)  /* Write-through */
#define PTE_CACHE_DIS   (1 << 4)   /* Cache disabled */
#define PTE_ACCESSED    (1 << 5)   /* Accessed */
#define PTE_DIRTY       (1 << 6)   /* Dirty */
#define PTE_HUGE        (1 << 7)   /* Huge page (1GB/2MB) */
#define PTE_GLOBAL      (1 << 8)   /* Global */
#define PTE_NX          (1ULL << 63) /* No Execute */

/* Page table index shifts */
#define PML4_SHIFT      39
#define PDP_SHIFT       30
#define PD_SHIFT        21
#define PT_SHIFT        12

/* Number of entries in each table level */
#define PTE_COUNT       512

/* Address space boundaries */
#define KERNEL_VIRT_BASE    0xFFFF800000000000ULL
#define KERNEL_VIRT_START   0xFFFFFFFF80000000ULL
#define USER_VIRT_START     0x0000000000000000ULL
#define USER_VIRT_END       0x00007FFFFFFFFFFFULL
#define VMALLOC_START       0xFFFF800000000000ULL
#define VMALLOC_END         0xFFFF87FFFFFFFFFFFULL

/* Page table entry type */
typedef uint64_t pte_t;
typedef pte_t pml4e_t;
typedef pte_t pdpe_t;
typedef pte_t pde_t;
typedef pte_t pte_t_actual;

/* Virtual address structure */
typedef struct __attribute__((packed)) {
    uint64_t offset : 12;
    uint64_t pt_index : 9;
    uint64_t pd_index : 9;
    uint64_t pdp_index : 9;
    uint64_t pml4_index : 9;
    uint64_t sign_ext : 27;
} virt_addr_t;

/*============================================================================
 * PAGE FAULT CODES
 *============================================================================*/

#define PF_PRESENT      (1 << 0)   /* Page not present */
#define PF_WRITE        (1 << 1)   /* Write access */
#define PF_USER         (1 << 2)   /* User mode */
#define PF_RESERVED     (1 << 3)   /* Reserved bit violation */
#define PF_INSTR        (1 << 4)   /* Instruction fetch */

/* Page fault error codes */
typedef struct {
    bool present : 1;
    bool write : 1;
    bool user : 1;
    bool reserved : 1;
    bool instr_fetch : 1;
    bool pk : 1;  /* Protection key */
    bool shadow_stack : 1;  /* Shadow stack access */
} page_fault_info_t;

/*============================================================================
 * PAGE FAULT STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t total_faults;
    uint64_t present_faults;
    uint64_t write_faults;
    uint64_t user_faults;
    uint64_t reserved_faults;
    uint64_t instruction_faults;
    uint64_t handled_faults;
    uint64_t unhandled_faults;
} page_fault_stats_t;

/*============================================================================
 * VIRTUAL MEMORY AREA (VMA) FLAGS
 *============================================================================*/

#define VMA_READ        0x1
#define VMA_WRITE       0x2
#define VMA_EXEC        0x4
#define VMA_USER        0x8
#define VMA_NOREGION    0x10
#define VMA_MMAP        0x20

/*============================================================================
 * VIRTUAL MEMORY AREA
 *============================================================================*/

typedef struct vma {
    uint64_t start;          /* Start address */
    uint64_t end;            /* End address */
    uint64_t flags;          /* VMA flags */
    const char *name;        /* Optional name */
    struct vma *next;        /* Linked list */
} vma_t;

/*============================================================================
 * VMM (VIRTUAL MEMORY MANAGER)
 *============================================================================*/

typedef struct {
    pml4e_t *pml4;          /* PML4 table physical address */
    uint64_t pml4_phys;     /* Physical address of PML4 */
    vma_t *vmas;            /* List of VMAs */
    uint64_t vmalloc_end;   /* End of vmalloc region */
    page_fault_stats_t stats;
    bool initialized;
} vmm_t;

/* Global VMM */
extern vmm_t *vmm;

/*============================================================================
 * PAGE TABLE OPERATIONS
 *============================================================================*/

/* Initialize paging */
int paging_init(void);

/* Get current PML4 address */
uint64_t paging_get_cr3(void);

/* Set PML4 address */
void paging_set_cr3(uint64_t pml4_phys);

/* Check if address is kernel address */
static inline bool is_kernel_address(uint64_t addr) {
    return (int64_t)addr < 0;
}

/* Check if address is user address */
static inline bool is_user_address(uint64_t addr) {
    return addr < 0x0000800000000000ULL;
}

/* Get page table indices from virtual address */
static inline void virt_to_indices(uint64_t virt, int *pml4_idx, int *pdp_idx, int *pd_idx, int *pt_idx) {
    *pml4_idx = (virt >> PML4_SHIFT) & 0x1FF;
    *pdp_idx = (virt >> PDP_SHIFT) & 0x1FF;
    *pd_idx = (virt >> PD_SHIFT) & 0x1FF;
    *pt_idx = (virt >> PT_SHIFT) & 0x1FF;
}

/* Create indices from address */
static inline uint64_t make_pte(uint64_t phys_addr, bool present, bool writable, bool user, bool nx) {
    uint64_t pte = phys_addr & ~0xFFF;
    if (present) pte |= PTE_PRESENT;
    if (writable) pte |= PTE_WRITABLE;
    if (user) pte |= PTE_USER;
    if (nx) pte |= PTE_NX;
    return pte;
}

/*============================================================================
 * PAGE ALLOCATION/DEALLOCATION
 *============================================================================*/

/* Map a page (virtual -> physical) */
int paging_map_page(uint64_t virt, uint64_t phys, bool writable, bool user, bool nx);

/* Unmap a page */
int paging_unmap_page(uint64_t virt);

/* Map range of pages */
int paging_map_range(uint64_t virt_start, uint64_t phys_start, size_t num_pages, 
                    bool writable, bool user, bool nx);

/* Unmap range of pages */
int paging_unmap_range(uint64_t virt_start, size_t num_pages);

/* Translate virtual address to physical */
uint64_t paging_virt_to_phys(uint64_t virt);

/* Check if virtual address is mapped */
bool paging_is_mapped(uint64_t virt);

/* Identity map a region */
int paging_identity_map(uint64_t start, uint64_t end, bool writable);

/*============================================================================
 * VMALLOC / VFREE
 *============================================================================*/

/* Allocate virtual memory (without physical backing) */
void *vmalloc(size_t size);

/* Free virtual memory */
void vfree(void *addr);

/* Allocate and map memory */
void *vmalloc_map(size_t size, uint64_t *phys_out);

/*============================================================================
 * PAGE FAULT HANDLER
 *============================================================================*/

/* Handle page fault */
int page_fault_handler(uint64_t fault_addr, uint32_t error_code);

/* Get page fault statistics */
page_fault_stats_t *paging_get_stats(void);

/* Reset statistics */
void paging_reset_stats(void);

/* Print statistics */
void paging_print_stats(void);

/*============================================================================
 * VMA MANAGEMENT
 *============================================================================*/

/* Find VMA containing address */
vma_t *vma_find(vmm_t *mm, uint64_t addr);

/* Insert VMA */
int vma_insert(vmm_t *mm, vma_t *vma);

/* Remove VMA */
int vma_remove(vmm_t *mm, vma_t *vma);

/*============================================================================
 * mmap/munmap (syscall stubs)
 *============================================================================*/

/* off_t for mmap syscall */
typedef int64_t off_t;

/* Memory map */
void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset);

/* Unmap memory */
int sys_munmap(void *addr, size_t length);

/* Change protection */
int sys_mprotect(void *addr, size_t length, int prot);

#endif /* FEATHEROS_PAGING_H */
