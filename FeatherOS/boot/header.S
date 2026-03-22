# FeatherOS - GRUB2 Multiboot2 Header
.section .bootstrap
.global _start

.set MULTIBOOT2_MAGIC, 0xE85250D6
.set MULTIBOOT2_ARCH, 0x9

.align 8
multiboot2_header:
    .int MULTIBOOT2_MAGIC
    .int MULTIBOOT2_ARCH
    .int 20
    .int -(0xE85250D6 + 0x9 + 20)

    .hword 2
    .hword 0
    .int 24
    .int 0x100000
    .int 0x100000
    .int 0
    .int 0

    .hword 3
    .hword 0
    .int 8
    .quad _start64

    .hword 0
    .hword 8
    .int 8

multiboot2_header_end:

.section .text.boot
.global _start
_start:
    jmp _start64

.section .text
.global _start64
_start64:
    movl %edi, multiboot_magic
    movq %rsi, multiboot_ptr
    xorl %eax, %eax
    movq $_bss_start, %rdi
    movq $_bss_end, %rcx
    subq %rdi, %rcx
    shrq $3, %rcx
    rep stosq
    movq $_stack_top, %rsp
    .extern kernel_main
    call kernel_main
hang:
    hlt
    jmp hang

.section .bss
.align 8
multiboot_magic: .space 8
multiboot_ptr: .space 8
_stack_bottom: .space 65536
_stack_top:
_bss_start: .space 8
_bss_end: .space 8

.section .rodata
.global _kernel_start
_kernel_start: .quad 0
.global _kernel_end
_kernel_end: .quad 0
