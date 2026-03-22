/*
 * FeatherOS - ELF64 Binary Loader
 * Loads ELF64 executables into user memory
 */

#include <stdint.h>
#include <stddef.h>

/* ELF64 constants */
#define ELF_MAGIC "\x7f" "ELF"
#define ELF_CLASS_64 2
#define ELF_DATA_LSB 1
#define ELF_TYPE_EXEC 2
#define ELF_MACHINE_X86_64 62

/* ELF64 header */
typedef struct {
    uint8_t  e_ident[16];     /* Magic number and other info */
    uint16_t e_type;          /* Object file type */
    uint16_t e_machine;       /* Architecture */
    uint32_t e_version;       /* Object file version */
    uint64_t e_entry;         /* Entry point virtual address */
    uint64_t e_phoff;         /* Program header table file offset */
    uint64_t e_shoff;         /* Section header table file offset */
    uint32_t e_flags;         /* Processor-specific flags */
    uint16_t e_ehsize;        /* ELF header size */
    uint16_t e_phentsize;     /* Program header table entry size */
    uint16_t e_phnum;         /* Program header table entry count */
    uint16_t e_shentsize;     /* Section header table entry size */
    uint16_t e_shnum;         /* Section header table entry count */
    uint16_t e_shstrndx;      /* Section header string table index */
} __attribute__((packed)) elf64_header_t;

/* ELF64 program header */
#define PT_LOAD 1

typedef struct {
    uint32_t p_type;     /* Segment type */
    uint32_t p_flags;   /* Segment flags */
    uint64_t p_offset;   /* Segment file offset */
    uint64_t p_vaddr;    /* Segment virtual address */
    uint64_t p_paddr;    /* Segment physical address */
    uint64_t p_filesz;   /* Segment size in file */
    uint64_t p_memsz;    /* Segment size in memory */
    uint64_t p_align;    /* Segment alignment */
} __attribute__((packed)) elf64_phdr_t;

/* Page size for alignment */
#define PAGE_SIZE 0x1000
#define PAGE_MASK (~(PAGE_SIZE - 1))

/* User space constants */
#define USER_CODE_VADDR 0x400000  /* User code base address */
#define USER_STACK_VADDR 0x7fffffff000  /* User stack */
#define USER_STACK_SIZE 0x200000  /* 2MB stack */

/* Memory allocation - stubs for now */
extern void* kmalloc(size_t size);
extern void kfree(void* ptr);

/* Page table operations - stubs */
extern void setup_user_page_tables(void);
extern void switch_to_user_page_tables(uint64_t pml4_phys);

/*
 * Validate ELF header
 */
static int elf_validate_header(elf64_header_t* hdr) {
    /* Check magic */
    if (hdr->e_ident[0] != 0x7f ||
        hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L' ||
        hdr->e_ident[3] != 'F') {
        return -1;  /* Not an ELF */
    }
    
    /* Check class (64-bit) */
    if (hdr->e_ident[4] != ELF_CLASS_64) {
        return -2;  /* Not 64-bit */
    }
    
    /* Check endianness (little-endian) */
    if (hdr->e_ident[5] != ELF_DATA_LSB) {
        return -3;  /* Not little-endian */
    }
    
    /* Check version */
    if (hdr->e_version != 1) {
        return -4;  /* Invalid version */
    }
    
    /* Check machine */
    if (hdr->e_machine != ELF_MACHINE_X86_64) {
        return -5;  /* Not x86_64 */
    }
    
    /* Check type (executable) */
    if (hdr->e_type != ELF_TYPE_EXEC) {
        return -6;  /* Not an executable */
    }
    
    return 0;  /* Valid */
}

/*
 * Load an ELF64 binary into user memory
 * 
 * Parameters:
 *   elf_data: Pointer to ELF file data
 *   size: Size of ELF data
 *   out_entry: Output pointer for entry point
 *   out_stack: Output pointer for initial stack pointer
 * 
 * Returns: 0 on success, negative on error
 */
int elf_load(void* elf_data, size_t size, uint64_t* out_entry, uint64_t* out_stack) {
    elf64_header_t* hdr = (elf64_header_t*)elf_data;
    elf64_phdr_t* phdr;
    uint64_t lowest_vaddr = 0xFFFFFFFFFFFFFFFF;
    uint64_t highest_vaddr = 0;
    uint64_t total_memsz = 0;
    void* user_code_base;
    int i;
    
    /* Validate header */
    if (elf_validate_header(hdr) != 0) {
        return -1;
    }
    
    /* Calculate memory requirements */
    phdr = (elf64_phdr_t*)((uint8_t*)elf_data + hdr->e_phoff);
    
    for (i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            if (phdr[i].p_vaddr < lowest_vaddr) {
                lowest_vaddr = phdr[i].p_vaddr;
            }
            if (phdr[i].p_vaddr + phdr[i].p_memsz > highest_vaddr) {
                highest_vaddr = phdr[i].p_vaddr + phdr[i].p_memsz;
            }
            total_memsz += phdr[i].p_memsz;
        }
    }
    
    /* Check if we fit at user code base */
    if (highest_vaddr > 0x7FFFFFFFFFFF) {
        return -10;  /* Too large for user space */
    }
    
    /* 
     * For simplicity, we load at USER_CODE_VADDR (0x400000)
     * This requires the memory at that location to be identity-mapped
     */
    user_code_base = (void*)USER_CODE_VADDR;
    
    /* Copy loadable segments */
    for (i = 0; i < hdr->e_phnum; i++) {
        if (phdr[i].p_type == PT_LOAD) {
            uint64_t src_offset = phdr[i].p_offset;
            uint64_t dst_vaddr = phdr[i].p_vaddr;
            uint64_t copy_size = phdr[i].p_filesz;
            
            /* Copy file data to user memory */
            void* dst_ptr = (uint8_t*)user_code_base + dst_vaddr;
            void* src_ptr = (uint8_t*)elf_data + src_offset;
            
            /* Copy with bounds check */
            if ((uintptr_t)src_ptr + copy_size > (uintptr_t)elf_data + size) {
                return -11;  /* Segment extends past EOF */
            }
            
            /* Copy the segment */
            for (uint64_t j = 0; j < copy_size; j++) {
                ((uint8_t*)dst_ptr)[j] = ((uint8_t*)src_ptr)[j];
            }
            
            /* Zero BSS (p_memsz - p_filesz) */
            for (uint64_t j = copy_size; j < phdr[i].p_memsz; j++) {
                ((uint8_t*)dst_ptr)[j] = 0;
            }
        }
    }
    
    /* Set output values */
    if (out_entry) {
        *out_entry = hdr->e_entry;  /* Entry point virtual address */
    }
    
    if (out_stack) {
        *out_stack = USER_STACK_VADDR + USER_STACK_SIZE - 8;  /* Stack top, 8-byte aligned */
    }
    
    return 0;  /* Success */
}

/*
 * Simple ELF loader that works with flat binary
 * (For programs that are just code, no ELF headers)
 */
int flat_binary_load(void* data, size_t size, uint64_t* out_entry, uint64_t* out_stack) {
    uint8_t* dest = (uint8_t*)USER_CODE_VADDR;
    
    /* Copy to user space */
    for (size_t i = 0; i < size; i++) {
        dest[i] = ((uint8_t*)data)[i];
    }
    
    /* Set entry to code base */
    if (out_entry) {
        *out_entry = USER_CODE_VADDR;
    }
    
    if (out_stack) {
        *out_stack = USER_STACK_VADDR + USER_STACK_SIZE - 8;
    }
    
    return 0;
}
