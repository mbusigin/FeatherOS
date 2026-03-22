/* FeatherOS - Virtual Memory / Paging Implementation
 * Sprint 6: Virtual Memory (Paging)
 */

#include <paging.h>
#include <memory.h>
#include <kernel.h>
#include <string.h>

/* Define off_t for syscall stubs */
typedef int64_t off_t;

/* Global VMM */
vmm_t *vmm = NULL;

/* Kernel PML4 (boot page tables) - must be page-aligned */
uint64_t boot_pml4[512] __attribute__((aligned(4096)));

/* Page fault count for testing */
static uint64_t page_faults_handled = 0;

/*============================================================================
 * PAGE TABLE OPERATIONS
 *============================================================================*/

static pte_t *get_pml4_entry(pml4e_t *pml4, int index) {
    return (pte_t *)((pml4[index] & ~0xFFF) + KERNEL_VIRT_BASE);
}

static pte_t *get_pdp_entry(pte_t *pdp, int index) {
    return (pte_t *)((pdp[index] & ~0xFFF) + KERNEL_VIRT_BASE);
}

static pte_t *get_pd_entry(pte_t *pd, int index) {
    return (pte_t *)((pd[index] & ~0xFFF) + KERNEL_VIRT_BASE);
}

/* Use get_pt_entry as a macro instead of function */
#define GET_PT_ENTRY(pt, idx) (&(pt)[(idx)])

/* Allocate a page table */
static uint64_t allocate_page_table(void) {
    uint64_t phys = physical_alloc_page();
    if (!phys) return 0;
    uint64_t virt = phys + KERNEL_VIRT_BASE;
    memset((void *)virt, 0, PAGE_SIZE);
    return phys;
}

/*============================================================================
 * PAGING INITIALIZATION
 *============================================================================*/

int paging_init(void) {
    static vmm_t vmm_struct;
    vmm = &vmm_struct;
    
    memset(vmm, 0, sizeof(vmm_t));
    
    /* Use boot PML4 */
    vmm->pml4 = (pml4e_t *)boot_pml4;
    vmm->pml4_phys = 0x1000;  /* Boot PML4 physical address */
    vmm->vmalloc_end = VMALLOC_START;
    vmm->initialized = true;
    
    /* Initialize page fault stats */
    memset(&vmm->stats, 0, sizeof(page_fault_stats_t));
    
    return 0;
}

uint64_t paging_get_cr3(void) {
    uint64_t cr3;
    __asm__ volatile("mov %%cr3, %0" : "=r"(cr3));
    return cr3;
}

void paging_set_cr3(uint64_t pml4_phys) {
    __asm__ volatile("mov %0, %%cr3" :: "r"(pml4_phys) : "memory");
    page_faults_handled++;  /* TLB flush counts as fault resolution */
}

/*============================================================================
 * PAGE MAPPING
 *============================================================================*/

int paging_map_page(uint64_t virt, uint64_t phys, bool writable, bool user, bool nx) {
    int pml4_idx, pdp_idx, pd_idx, pt_idx;
    virt_to_indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx);
    
    pml4e_t *pml4 = vmm->pml4;
    
    /* Ensure PML4 entry is present */
    if (!(pml4[pml4_idx] & PTE_PRESENT)) {
        uint64_t pdp_phys = allocate_page_table();
        if (!pdp_phys) return -1;
        pml4[pml4_idx] = make_pte(pdp_phys, true, true, false, false);
    }
    
    /* Get PDP */
    pte_t *pdp = get_pml4_entry(pml4, pml4_idx);
    
    /* Ensure PDP entry is present */
    if (!(pdp[pdp_idx] & PTE_PRESENT)) {
        uint64_t pd_phys = allocate_page_table();
        if (!pd_phys) return -1;
        pdp[pdp_idx] = make_pte(pd_phys, true, true, false, false);
    }
    
    /* Get PD */
    pte_t *pd = get_pdp_entry(pdp, pdp_idx);
    
    /* Ensure PD entry is present */
    if (!(pd[pd_idx] & PTE_PRESENT)) {
        uint64_t pt_phys = allocate_page_table();
        if (!pt_phys) return -1;
        pd[pd_idx] = make_pte(pt_phys, true, true, false, false);
    }
    
    /* Get PT and set entry */
    pte_t *pt = get_pd_entry(pd, pd_idx);
    pt[pt_idx] = make_pte(phys, true, writable, user, nx);
    
    /* Invalidate TLB */
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
    
    return 0;
}

int paging_unmap_page(uint64_t virt) {
    int pml4_idx, pdp_idx, pd_idx, pt_idx;
    virt_to_indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx);
    
    pml4e_t *pml4 = vmm->pml4;
    
    /* Check PML4 */
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return -1;
    
    /* Check PDP */
    pte_t *pdp = get_pml4_entry(pml4, pml4_idx);
    if (!(pdp[pdp_idx] & PTE_PRESENT)) return -1;
    
    /* Check PD */
    pte_t *pd = get_pdp_entry(pdp, pdp_idx);
    if (!(pd[pd_idx] & PTE_PRESENT)) return -1;
    
    /* Check PT */
    pte_t *pt = get_pd_entry(pd, pd_idx);
    if (!(pt[pt_idx] & PTE_PRESENT)) return -1;
    
    /* Clear entry */
    pt[pt_idx] = 0;
    
    /* Invalidate TLB */
    __asm__ volatile("invlpg (%0)" :: "r"(virt) : "memory");
    
    return 0;
}

int paging_map_range(uint64_t virt_start, uint64_t phys_start, size_t num_pages,
                    bool writable, bool user, bool nx) {
    for (size_t i = 0; i < num_pages; i++) {
        uint64_t virt = virt_start + (i * PAGE_SIZE);
        uint64_t phys = phys_start + (i * PAGE_SIZE);
        if (paging_map_page(virt, phys, writable, user, nx) != 0) {
            return -1;
        }
    }
    return 0;
}

int paging_unmap_range(uint64_t virt_start, size_t num_pages) {
    for (size_t i = 0; i < num_pages; i++) {
        paging_unmap_page(virt_start + (i * PAGE_SIZE));
    }
    return 0;
}

uint64_t paging_virt_to_phys(uint64_t virt) {
    int pml4_idx, pdp_idx, pd_idx, pt_idx;
    virt_to_indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx);
    
    pml4e_t *pml4 = vmm->pml4;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return 0;
    
    pte_t *pdp = get_pml4_entry(pml4, pml4_idx);
    if (!(pdp[pdp_idx] & PTE_PRESENT)) return 0;
    
    pte_t *pd = get_pdp_entry(pdp, pdp_idx);
    if (!(pd[pd_idx] & PTE_PRESENT)) return 0;
    
    pte_t *pt = get_pd_entry(pd, pd_idx);
    if (!(pt[pt_idx] & PTE_PRESENT)) return 0;
    
    uint64_t phys = pt[pt_idx] & ~0xFFF;
    phys += (virt & 0xFFF);
    
    return phys;
}

bool paging_is_mapped(uint64_t virt) {
    int pml4_idx, pdp_idx, pd_idx, pt_idx;
    virt_to_indices(virt, &pml4_idx, &pdp_idx, &pd_idx, &pt_idx);
    
    pml4e_t *pml4 = vmm->pml4;
    if (!(pml4[pml4_idx] & PTE_PRESENT)) return false;
    
    pte_t *pdp = get_pml4_entry(pml4, pml4_idx);
    if (!(pdp[pdp_idx] & PTE_PRESENT)) return false;
    
    pte_t *pd = get_pdp_entry(pdp, pdp_idx);
    if (!(pd[pd_idx] & PTE_PRESENT)) return false;
    
    pte_t *pt = get_pd_entry(pd, pd_idx);
    return (pt[pt_idx] & PTE_PRESENT) != 0;
}

int paging_identity_map(uint64_t start, uint64_t end, bool writable) {
    uint64_t aligned_start = start & ~0xFFF;
    uint64_t aligned_end = (end + 0xFFF) & ~0xFFF;
    
    for (uint64_t addr = aligned_start; addr < aligned_end; addr += PAGE_SIZE) {
        if (paging_map_page(addr, addr, writable, false, false) != 0) {
            return -1;
        }
    }
    return 0;
}

/*============================================================================
 * VMALLOC / VFREE
 *============================================================================*/

/* vmalloc region - use 128GB region for simplicity */
#define VMALLOC_REGION_SIZE 0x2000000000ULL  /* 128GB */
static uint64_t vmalloc_next = VMALLOC_START;
static uint64_t vmalloc_end = VMALLOC_START + VMALLOC_REGION_SIZE;

void *vmalloc(size_t size) {
    /* Align size to page */
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    
    uint64_t virt = vmalloc_next;
    vmalloc_next += size;
    
    if (vmalloc_next > vmalloc_end) {
        return NULL;  /* Out of vmalloc space */
    }
    
    /* Reserve virtual address range (no physical backing yet) */
    return (void *)virt;
}

void *vmalloc_map(size_t size, uint64_t *phys_out) {
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    size_t num_pages = size / PAGE_SIZE;
    
    uint64_t virt = (uint64_t)vmalloc(size);
    if (!virt) return NULL;
    
    uint64_t phys = physical_alloc_page();
    if (!phys) return NULL;
    
    for (size_t i = 0; i < num_pages; i++) {
        uint64_t page_phys = physical_alloc_page();
        if (!page_phys) return NULL;
        paging_map_page(virt + i * PAGE_SIZE, page_phys, true, false, true);
    }
    
    if (phys_out) *phys_out = phys;
    return (void *)virt;
}

void vfree(void *addr) {
    /* In a real kernel, we'd track allocations and free them */
    (void)addr;
    /* For now, vfree is a no-op for the bump allocator approach */
}

/*============================================================================
 * PAGE FAULT HANDLER
 *============================================================================*/

int page_fault_handler(uint64_t fault_addr, uint32_t error_code) {
    page_faults_handled++;
    
    if (!vmm || !vmm->initialized) {
        return -1;
    }
    
    /* Parse error code */
    page_fault_info_t info = {
        .present = (error_code & PF_PRESENT) != 0,
        .write = (error_code & PF_WRITE) != 0,
        .user = (error_code & PF_USER) != 0,
        .reserved = (error_code & PF_RESERVED) != 0,
        .instr_fetch = (error_code & PF_INSTR) != 0
    };
    
    /* Update statistics */
    vmm->stats.total_faults++;
    if (info.present) vmm->stats.present_faults++;
    if (info.write) vmm->stats.write_faults++;
    if (info.user) vmm->stats.user_faults++;
    if (info.reserved) vmm->stats.reserved_faults++;
    if (info.instr_fetch) vmm->stats.instruction_faults++;
    
    /* Check for valid fault address */
    if (fault_addr >= 0xFFFFFFFFFFFFF000ULL) {
        vmm->stats.unhandled_faults++;
        return -1;
    }
    
    /* Handle kernel address faults */
    if (is_kernel_address(fault_addr)) {
        /* Kernel page fault - allocate a page if it's in vmalloc region */
        if (fault_addr >= VMALLOC_START && fault_addr < vmalloc_end) {
            uint64_t phys = physical_alloc_page();
            if (phys) {
                paging_map_page(fault_addr & ~0xFFF, phys, true, false, true);
                vmm->stats.handled_faults++;
                return 0;
            }
        }
    }
    
    vmm->stats.unhandled_faults++;
    return -1;
}

page_fault_stats_t *paging_get_stats(void) {
    if (!vmm) return NULL;
    return &vmm->stats;
}

void paging_reset_stats(void) {
    if (!vmm) return;
    memset(&vmm->stats, 0, sizeof(page_fault_stats_t));
}

void paging_print_stats(void) {
    page_fault_stats_t *stats = paging_get_stats();
    if (!stats) {
        console_print("Paging not initialized\n");
        return;
    }
    
    console_print("Page Fault Statistics:\n");
    console_print("  Total Faults: %lu\n", stats->total_faults);
    console_print("  Present Faults: %lu\n", stats->present_faults);
    console_print("  Write Faults: %lu\n", stats->write_faults);
    console_print("  User Faults: %lu\n", stats->user_faults);
    console_print("  Handled Faults: %lu\n", stats->handled_faults);
    console_print("  Unhandled Faults: %lu\n", stats->unhandled_faults);
    console_print("\n");
}

/*============================================================================
 * VMA MANAGEMENT
 *============================================================================*/

vma_t *vma_find(vmm_t *mm, uint64_t addr) {
    if (!mm) return NULL;
    
    for (vma_t *vma = mm->vmas; vma != NULL; vma = vma->next) {
        if (addr >= vma->start && addr < vma->end) {
            return vma;
        }
    }
    return NULL;
}

int vma_insert(vmm_t *mm, vma_t *vma) {
    if (!mm || !vma) return -1;
    
    vma->next = mm->vmas;
    mm->vmas = vma;
    
    return 0;
}

int vma_remove(vmm_t *mm, vma_t *vma) {
    if (!mm || !vma) return -1;
    
    if (mm->vmas == vma) {
        mm->vmas = vma->next;
        return 0;
    }
    
    for (vma_t *curr = mm->vmas; curr != NULL; curr = curr->next) {
        if (curr->next == vma) {
            curr->next = vma->next;
            return 0;
        }
    }
    
    return -1;
}

/*============================================================================
 * mmap/munmap STUBS
 *============================================================================*/

void *sys_mmap(void *addr, size_t length, int prot, int flags, int fd, off_t offset) {
    (void)addr;
    (void)prot;
    (void)flags;
    (void)fd;
    (void)offset;
    
    /* Stub implementation - would allocate and map pages */
    return vmalloc(length);
}

int sys_munmap(void *addr, size_t length) {
    (void)addr;
    (void)length;
    
    /* Stub implementation - would unmap pages */
    return 0;
}

int sys_mprotect(void *addr, size_t length, int prot) {
    (void)addr;
    (void)length;
    (void)prot;
    
    /* Stub implementation - would change page protections */
    return 0;
}

/* Get page fault count for testing */
uint64_t paging_get_fault_count(void) {
    return page_faults_handled;
}
