# FeatherOS - User Space Entry
# Sets up user page tables, segment selectors, and returns to user mode via iret

.section .text

#============================================================================
# Constants
#============================================================================

# User virtual address layout
.equ USER_CODE_VADDR,   0x400000    # User code base
.equ USER_STACK_VADDR,  0x7fffffff000 # User stack (high address)
.equ USER_STACK_SIZE,   0x200000    # 2MB stack

# GDT selectors
.equ KERNEL_CS,        0x08        # Kernel code segment
.equ KERNEL_DS,        0x10        # Kernel data segment  
.equ USER_CS,          0x18        # User code segment (RPL=3)
.equ USER_DS,          0x20        # User data segment (RPL=3)

# Paging flags
.equ PTE_PRESENT,      0x01        # Present
.equ PTE_WRITABLE,     0x02        # Read/Write
.equ PTE_USER,         0x04        # User accessible
.equ PTE_WRITE_THROUGH, 0x08       # Write-through
.equ PTE_CACHE_DISABLE, 0x10        # Cache disabled
.equ PTE_ACCESSED,     0x20        # Accessed
.equ PTE_DIRTY,        0x40        # Dirty
.equ PTE_LARGE,        0x80        # 2MB page
.equ PTE_NX,           0x8000000000000000 # No-execute (needs PAE/IA-32e)

#============================================================================
# Global Symbols
#============================================================================

.globl user_space_entry
.globl setup_user_page_tables
.globl user_page_table_root

#============================================================================
# User Space Entry Point
#
# Called from kernel to start a user program
# Inputs:
#   rdi = user entry point (RIP)
#   rsi = user stack pointer
#   rdx = page table root (PML4 physical address)
#============================================================================

user_space_entry:
    # Save kernel state (we'll return via iret)
    pushq   $USER_CS              # SS - user data segment
    pushq   %rsi                 # RSP - user stack pointer
    pushq   $0x200               # RFLAGS - IF set
    pushq   $USER_CS              # CS - user code segment
    pushq   %rdi                 # RIP - user entry point
    
    # Switch to user page tables
    movq    %rdx, %cr3
    
    # Load user data segment registers
    movw    $USER_DS, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    
    # Return to user mode via iret
    iretq

#============================================================================
# Set Up User Page Tables
#
# Creates page tables that map:
# - 0x000000000000 - 0x00007fffffffffff: kernel space (identity)
# - 0x0000800000000000 - 0xffff7fffffffffff: invalid
# - 0x0001000000000000 - 0x00017fffffffffff: kernel space (high)
# - 0x0000000000400000 - 0x00007fffffffffff: user space
#
# For simplicity, we create a minimal 1GB user mapping using 2MB pages
#============================================================================

setup_user_page_tables:
    pushq   %rbp
    movq    %rsp, %rbp
    
    # Allocate page table memory (this should be provided by caller)
    # For now, assume the page tables are pre-allocated
    # RDI = pointer to PML4
    
    movq    %rdi, %rax
    
    # Clear PML4 (first entry points to PDP)
    movq    $0, (%rax)
    
    # Set up user page mapping at PML4 entry 0
    # Point to PDP table (allocated at rax + 0x1000)
    movq    %rax, %rbx           # Base address
    addq    $0x1000, %rbx        # PDP table offset
    orq     $PTE_PRESENT, %rbx   # Present
    orq     $PTE_WRITABLE, %rbx  # Writable
    orq     $PTE_USER, %rbx      # User accessible
    movq    %rbx, (%rax)         # Store in PML4[0]
    
    # Clear PDP table (at rax + 0x1000)
    pushq   %rax
    movq    %rax, %rdi
    addq    $0x1000, %rdi
    movq    $0x1000, %rcx        # 256 entries * 8 bytes
    xorq    %rax, %rax
    rep stosb
    
    popq    %rax
    
    # Set up PDP entry 0 - point to PD table
    addq    $0x2000, %rbx        # PD table offset
    orq     $PTE_PRESENT, %rbx
    orq     $PTE_WRITABLE, %rbx
    orq     $PTE_USER, %rbx
    movq    %rbx, 0x1000(%rax)   # Store in PDP[0]
    
    # Clear PD table (at rax + 0x2000)
    pushq   %rax
    movq    %rax, %rdi
    addq    $0x2000, %rdi
    movq    $0x1000, %rcx        # 512 entries * 8 bytes = 4KB
    xorq    %rax, %rax
    rep stosb
    
    popq    %rax
    
    # Set up PD entries for user space
    # Map 0x400000 to 0x7fffffffff (2GB user space) using 2MB pages
    # Each PD entry covers 2MB
    # Start at offset 0x400000 / 0x200000 = 2nd entry
    
    movq    %rax, %rbx           # Base of page tables
    addq    $0x2000, %rbx        # PD table
    
    # Map 512 x 2MB = 1GB starting at user address 0
    # For our shell at 0x400000, we start at PD[0]
    # PD[0] = physical address 0x100000 | flags
    movq    $0x100000, %rcx      # Physical base for user memory
    orq     $PTE_PRESENT, %rcx
    orq     $PTE_WRITABLE, %rcx
    orq     $PTE_USER, %rcx
    orq     $PTE_LARGE, %rcx    # 2MB page
    movq    %rcx, 0x2000(%rax)   # PD[0] = 0x0 -> 0x200000
    
    # Map another 511 entries for remaining user space
    movq    $1, %r8              # Counter
setup_pd_entries:
    movq    $0x100000, %rcx      # Physical base
    imulq   $0x200, %r8, %rcx    # Each 2MB = 512 * 2MB = 1GB total
    addq    $0x200000, %rcx      # Offset by 2MB for this entry
    orq     $PTE_PRESENT, %rcx
    orq     $PTE_WRITABLE, %rcx
    orq     $PTE_USER, %rcx
    orq     $PTE_LARGE, %rcx
    leaq    0x2000(%rax, %r8, 8), %r9  # Address of PD[r8]
    movq    %rcx, (%r9)          # PD[r8] = entry
    incq    %r8
    cmpq    $512, %r8
    jl      setup_pd_entries
    
    # Return PML4 address in RAX
    popq    %rbp
    retq

#============================================================================
# Set Up Kernel-Only Mapping for User Page Tables
#
# When running user processes, we need the kernel to be mapped in the
# upper half of the address space (0xFFFFFFFF80000000 onwards)
# This function sets up the higher half mapping
#============================================================================

setup_kernel_high_mapping:
    pushq   %rbp
    movq    %rsp, %rbp
    
    # RDI = PML4 pointer
    # Set PML4 entry 255 (0xFF) to map kernel at high addresses
    movq    %rdi, %rax
    movq    $0x1000, %rbx        # PDP table for kernel high
    orq     $PTE_PRESENT, %rbx
    orq     $PTE_WRITABLE, %rbx
    # No USER flag = kernel only
    movq    %rbx, -8(%rax)       # PML4[255] = kernel PDP
    
    popq    %rbp
    retq

#============================================================================
# Data Section - Page Table Root
#============================================================================

.section .data
.align 4096

user_page_table_root:
    .space 4096 * 4    # PML4 + PDP + PD + PT (16KB)

#============================================================================
# Create User Stack
#
# Sets up a user stack at the high address
#============================================================================

.globl setup_user_stack
setup_user_stack:
    # RDI = stack virtual address (typically 0x7fffffff000)
    # RSI = stack size
    
    # Allocate physical pages for stack
    # For now, return the virtual address (pages pre-allocated)
    movq    %rdi, %rax
    retq

#============================================================================
# Switch to User Mode (Full Version)
#
# Complete transition to user mode with all required state
# Inputs:
#   RDI = user entry RIP
#   RSI = user stack RSP
#   RDX = page table root (physical)
#   RCX = argc
#   R8  = argv pointer
#   R9  = envp pointer
#============================================================================

.globl enter_user_mode
enter_user_mode:
    # Save arguments
    movq    %rdi, %r15          # Save entry RIP
    movq    %rsi, %r14          # Save stack RSP
    movq    %rdx, %r13          # Save page table root
    
    # Push iret frame onto current stack
    pushq   $USER_DS            # SS
    pushq   %r14                # RSP
    pushq   $0x200              # RFLAGS (IF=1)
    pushq   $USER_CS            # CS
    pushq   %r15                # RIP
    
    # Set up argc, argv, envp on user stack
    movq    %r14, %rsp          # Switch to user stack
    
    # Push environment
    movq    %r9, %rax
    testq   %rax, %rax
    jz      .no_envp
    pushq   %rax                # envp
.no_envp:
    
    # Push argv
    movq    %r8, %rax
    testq   %rax, %rax
    jz      .no_argv
    pushq   %rax                # argv
.no_argv:
    
    # Push argc
    movq    %rcx, %rax
    pushq   %rax                # argc
    
    # Align stack to 16 bytes
    andq    $~0xF, %rsp
    
    # Switch page tables
    movq    %r13, %cr3
    
    # Load user segment registers
    movw    $USER_DS, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    
    # Clear kernel GS base (we don't use it in userspace)
    xorq    %rax, %rax
    movq    %rax, %gs:0
    
    # Return to user mode
    iretq
