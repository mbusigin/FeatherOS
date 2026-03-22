# FeatherOS Syscall Entry/Exit - x86_64
# This file provides the syscall entry point and helper functions

.section .text

# Syscall entry point - called via interrupt or syscall instruction
# Saves user state, calls do_syscall, restores state
.globl syscall_entry
.type syscall_entry, @function
syscall_entry:
    /* Save all registers - syscall clobbers rcx and r11 */
    push %rax        /* Save syscall number (will be return value) */
    push %rcx        /* Save original RIP (clobbered by syscall) */
    push %r11        /* Save original RFLAGS (clobbered by syscall) */
    push %rdi        /* Save arg1 */
    push %rsi        /* Save arg2 */
    push %rdx        /* Save arg3 */
    push %r10        /* Save arg4 (syscall uses r10, not rcx) */
    push %r8         /* Save arg5 */
    push %r9         /* Save arg6 */
    push %rbp
    push %rbx
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Load syscall handler from pointer */
    movq sycall_handler_ptr(%rip), %rax
    testq %rax, %rax
    jz syscall_return
    
    /* Set up args for handler call
     * syscall convention: rdi=syscall#, rsi=arg2, rdx=arg3, r10=arg4, r8=arg5, r9=arg6 */
    movq 120(%rsp), %rdi    /* syscall number -> rdi */
    
    /* Call syscall handler - returns result in rax */
    callq *%rax
    
syscall_return:
    /* Result is now in rax - restore state (reverse order) */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    pop %r9
    pop %r8
    pop %r10
    pop %rdx
    pop %rsi
    pop %rdi
    pop %r11        /* Restore original RFLAGS */
    pop %rcx        /* Restore original RIP */
    pop %rax        /* Syscall return value */
    
    iretq

.size syscall_entry, .-syscall_entry

# Syscall return helper (for C code to call)
.globl syscall_return_from_handler
.type syscall_return_from_handler, @function
syscall_return_from_handler:
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %rbx
    pop %rbp
    pop %r9
    pop %r8
    pop %r10
    pop %rdx
    pop %rsi
    pop %rdi
    pop %r11
    pop %rcx
    pop %rax
    iretq

.size syscall_return_from_handler, .-syscall_return_from_handler

# Reference to syscall handler pointer (defined in interrupt.S or elsewhere)
sycall_handler_ptr:
    .quad 0
