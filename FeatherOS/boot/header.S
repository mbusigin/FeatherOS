# FeatherOS - Boot Code
# GRUB2 multiboot bootloader

# Multiboot header at the very beginning of the file
.section .multiboot
.align 8
multiboot_header:
    .int 0xE85250D6     # Magic
    .int 0             # Architecture (32-bit protected mode)
    .int 24            # Header length
    .int 0x100000000 - (0xE85250D6 + 24)
    .hword 0
    .hword 0
    .int 8

# ELF note for boot info
.section .note
.align 4
.elf_note:
    .int 5              # namesz
    .int 24             # descsz
    .int 32             # type
    .asciz "Linux"
    .int 0x100000       # phys base
    .quad _start64      # entry

.section .text
.code32
.global _start
.type _start, @function
_start:
    # GRUB jumps here in 32-bit protected mode
    # We need to switch to 64-bit long mode
    
    # Load new GDT
    lgdt gdt_ptr
    
    # Set up segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    # Set up stack
    movl $_stack_top, %esp
    
    # Enable PAE
    movl %cr4, %eax
    orl $0x20, %eax
    movl %eax, %cr4
    
    # Point CR3 to PML4
    movl $pml4, %eax
    movl %eax, %cr3
    
    # Enable long mode in EFER
    movl $0xC0000080, %ecx
    rdmsr
    orl $0x100, %eax
    wrmsr
    
    # Enable paging (CR0.PG)
    movl %cr0, %eax
    orl $0x80000000, %eax
    movl %eax, %cr0
    
    # Far jump to 64-bit code
    ljmp $0x08, $_start64

# 64-bit code starts here
.code64
.global _start64
.type _start64, @function
_start64:
    # Set up data segments
    movw $0x10, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    # Set up stack
    movq $_stack_top, %rsp
    xorq %rbp, %rbp
    
    # Clear BSS
    xorq %rax, %rax
    movq $_bss_start, %rdi
    movq $_bss_end, %rcx
    subq %rdi, %rcx
    shrq $3, %rcx
    cld
    rep stosq
    
    # Output boot message to serial
    leaq serial_msg(%rip), %rdi
    call serial_puts
    
    # Call kernel main
    call kernel_main
    
halt_loop:
    hlt
    jmp halt_loop

# Serial output function
serial_puts:
    movq %rdi, %rbx
    xorq %rcx, %rcx
serial_puts_loop:
    movb (%rbx, %rcx, 1), %al
    testb %al, %al
    jz serial_puts_done
    call serial_putchar
    incq %rcx
    jmp serial_puts_loop
serial_puts_done:
    ret

serial_putchar:
    pushq %rax
    pushq %rdx
    movb %al, %dl
    movl $0x3F8, %eax
serial_wait:
    inb %dx, %al
    testb $0x20, %al
    jz serial_wait
    movl $0x3F8, %eax
    movb %dl, %al
    outb %al, %dx
    popq %rdx
    popq %rax
    ret

serial_msg:
    .asciz "FeatherOS booting via GRUB...\r\n\0"

# GDT
.align 16
gdt:
    .quad 0
    .quad 0x0020989A000000FFFF   # Kernel code
    .quad 0x00209092000000FFFF   # Kernel data
gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .long gdt

# Page tables
.align 4096
pml4:
    .long pdpt + 0x03, 0

pdpt:
    .long pd + 0x03, 0

pd:
    .long 0x00000083, 0         # 4MB page, present, writable
    .fill 509, 4, 0

.section .bss
.align 16
_stack_bottom:
    .space 65536
_stack_top:

_bss_start:
    .long 0, 0
_bss_end:
    .long 0, 0
