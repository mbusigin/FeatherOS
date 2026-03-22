/* FeatherOS - Interrupt Service Routines
 * Sprint 13: Interrupt Handling
 */

#include "asm_macro.h"

.section .text

/* Macro to create an interrupt gate */
.macro INTERRUPT_GATE num
    .globl irq_entry_\num
irq_entry_\num:
    /* Push dummy error code if not present */
    push $\num
    jmp common_interrupt
.endm

/* Macro to create exception with error code */
.macro EXCEPTION_WITH_CODE num
    .globl exception_entry_\num
exception_entry_\num:
    jmp common_exception
.endm

/* Macro to create exception without error code */
.macro EXCEPTION_NO_CODE num
    .globl exception_entry_\num
exception_entry_\num:
    /* Push dummy error code */
    push $0
    jmp common_exception
.endm

/*============================================================================
 * EXCEPTION HANDLERS (0-31)
 *============================================================================*/

EXCEPTION_NO_CODE  0   /* DE - Division Error */
EXCEPTION_NO_CODE  1   /* DB - Debug */
EXCEPTION_NO_CODE  2   /* NMI */
EXCEPTION_NO_CODE  3   /* BP - Breakpoint */
EXCEPTION_NO_CODE  4   /* OF - Overflow */
EXCEPTION_NO_CODE  5   /* BR - Bound Range */
EXCEPTION_NO_CODE  6   /* UD - Invalid Opcode */
EXCEPTION_NO_CODE  7   /* NM - Device Not Available */
EXCEPTION_WITH_CODE 8  /* DF - Double Fault */
EXCEPTION_NO_CODE  9   /* Coprocessor Segment Overrun */
EXCEPTION_WITH_CODE 10 /* TS - Invalid TSS */
EXCEPTION_WITH_CODE 11 /* NP - Segment Not Present */
EXCEPTION_WITH_CODE 12 /* SS - Stack Fault */
EXCEPTION_WITH_CODE 13 /* GP - General Protection */
EXCEPTION_WITH_CODE 14 /* PF - Page Fault */
EXCEPTION_NO_CODE  15  /* Reserved */
EXCEPTION_NO_CODE  16  /* MF - Math Fault */
EXCEPTION_NO_CODE  17  /* AC - Alignment Check */
EXCEPTION_NO_CODE  18  /* MC - Machine Check */
EXCEPTION_NO_CODE  19  /* XF - SIMD */

/* 20-31 reserved */
EXCEPTION_NO_CODE  20
EXCEPTION_NO_CODE  21
EXCEPTION_NO_CODE  22
EXCEPTION_NO_CODE  23
EXCEPTION_NO_CODE  24
EXCEPTION_NO_CODE  25
EXCEPTION_NO_CODE  26
EXCEPTION_NO_CODE  27
EXCEPTION_NO_CODE  28
EXCEPTION_NO_CODE  29
EXCEPTION_NO_CODE  30
EXCEPTION_NO_CODE  31

/*============================================================================
 * IRQ HANDLERS (0-15 mapped to 32-47)
 *============================================================================*/

INTERRUPT_GATE 32  /* IRQ0 - Timer */
INTERRUPT_GATE 33  /* IRQ1 - Keyboard */
INTERRUPT_GATE 34  /* IRQ2 - Cascade */
INTERRUPT_GATE 35  /* IRQ3 - COM2 */
INTERRUPT_GATE 36  /* IRQ4 - COM1 */
INTERRUPT_GATE 37  /* IRQ5 - LPT2 */
INTERRUPT_GATE 38  /* IRQ6 - Floppy */
INTERRUPT_GATE 39  /* IRQ7 - LPT1/Spurious */
INTERRUPT_GATE 40  /* IRQ8 - RTC */
INTERRUPT_GATE 41  /* IRQ9 - Free */
INTERRUPT_GATE 42  /* IRQ10 - Free */
INTERRUPT_GATE 43  /* IRQ11 - Free */
INTERRUPT_GATE 44  /* IRQ12 - PS/2 Mouse */
INTERRUPT_GATE 45  /* IRQ13 - FPU */
INTERRUPT_GATE 46  /* IRQ14 - Primary IDE */
INTERRUPT_GATE 47  /* IRQ15 - Secondary IDE */

/*============================================================================
 * SYSTEM CALL INTERRUPT
 *============================================================================*/

.globl syscall_entry
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
    
    /* Load syscall handler */
    movq sycall_handler_ptr(%rip), %rax
    testq %rax, %rax
    jz syscall_return
    
    /* Restore args to registers for handler call
     * syscall convention: rdi=arg1, rsi=arg2, rdx=arg3, r10=arg4, r8=arg5, r9=arg6
     * syscall number was in original rax (now at rsp+120) */
    movq 120(%rsp), %rdi    /* syscall number -> rdi */
    movq %rsi, %rsi         /* arg2 */
    movq %rdx, %rdx         /* arg3 */
    /* r10 already has arg4 */
    /* r8 already has arg5 */
    /* r9 already has arg6 */
    
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
    pop %rcx        /* Restore original RIP -> goes to rcx */
    pop %rax        /* Syscall return value */
    
    iretq

sycall_handler_ptr:
    .quad 0

/*============================================================================
 * COMMON INTERRUPT HANDLER
 *============================================================================*/

common_interrupt:
    /* Save all registers */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Get interrupt number from stack */
    movq 16(%rsp), %rax  /* After pushed registers + original IP */
    
    /* Check if we need to adjust for vector > 31 */
    cmp $31, %rax
    jg 1f
    
    /* Exception handler - use exception handler array */
    movq exception_handler_ptr(%rip), %rbx
    testq %rbx, %rbx
    jz 1f
    
    /* Get handler for this exception */
    leaq (%rbx, %rax, 8), %rbx
    movq (%rbx), %rbx
    testq %rbx, %rbx
    jnz 1f
    
    /* No handler, halt */
    hlt
    jmp .
    
1:
    /* Call interrupt handler */
    movq interrupt_handler_ptr(%rip), %rbx
    testq %rbx, %rbx
    jz 2f
    
    /* Get interrupt number */
    movq 16(%rsp), %rdi
    
    /* Setup frame pointer */
    leaq 16(%rsp), %rsi
    
    /* Call handler */
    callq *%rbx
    
2:
    /* Check if we should reschedule */
    movq should_schedule_ptr(%rip), %rax
    testq %rax, %rax
    jz 3f
    
    callq *%rax
    /* After schedule, restore state and return from interrupt properly */
    /* Fall through to 3f */
    
3:
    /* Restore registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    
    /* Get interrupt number */
    movq 0(%rsp), %rax
    cmp $31, %rax
    jg irq_dispatcher
    
    addq $8, %rsp  /* Skip error code for exceptions */
    iretq

irq_dispatcher:
    /* Send EOI to PIC/APIC */
    movq eoi_handler_ptr(%rip), %rax
    testq %rax, %rax
    jz 1f
    
    /* Calculate IRQ from vector (IRQ 0-15 = vectors 32-47) */
    sub $32, %rax  /* We pass IRQ number in rdi */
    callq *%rax
    
1:
    iretq

/*============================================================================
 * COMMON EXCEPTION HANDLER
 *============================================================================*/

common_exception:
    /* Save all registers */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Call exception handler */
    movq exception_handler_ptr(%rip), %rbx
    testq %rbx, %rbx
    jz 1f
    
    /* Get exception number */
    movq 16(%rsp), %rdi
    
    /* Setup frame pointer */
    leaq 16(%rsp), %rsi
    
    /* Call handler */
    callq *%rbx
    
1:
    /* Restore registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    
    /* Skip error code and return */
    addq $8, %rsp
    iretq

/* Handler pointers (set by C code) */
exception_handler_ptr:
    .quad 0

interrupt_handler_ptr:
    .quad 0

eoi_handler_ptr:
    .quad 0

should_schedule_ptr:
    .quad 0

/*============================================================================
 * PAGE FAULT HANDLER (Special - needs fault address)
 *============================================================================*/

.globl page_fault_entry
page_fault_entry:
    /* Get fault address from CR2 */
    movq %cr2, %rax
    
    /* Get error code from stack */
    movq 0(%rsp), %rsi
    
    /* Save frame */
    push %rax
    push %rbx
    push %rcx
    push %rdx
    push %rbp
    push %rsi
    push %rdi
    push %r8
    push %r9
    push %r10
    push %r11
    push %r12
    push %r13
    push %r14
    push %r15
    
    /* Call C page fault handler */
    movq page_fault_c_handler(%rip), %rax
    testq %rax, %rax
    jz 1f
    
    movq %cr2, %rdi
    movq 120(%rsp), %rsi  /* Error code */
    callq *%rax
    
1:
    /* Restore registers */
    pop %r15
    pop %r14
    pop %r13
    pop %r12
    pop %r11
    pop %r10
    pop %r9
    pop %r8
    pop %rdi
    pop %rsi
    pop %rbp
    pop %rdx
    pop %rcx
    pop %rbx
    pop %rax
    
    iretq

page_fault_c_handler:
    .quad 0

/*============================================================================
 * LOAD IDT
 *============================================================================*/

.globl load_idt
.type load_idt, @function
load_idt:
    lidt (%rdi)
    ret

/*============================================================================
 * INTERRUPT ENABLE/DISABLE
 *============================================================================*/

.globl enable_interrupts_func
.type enable_interrupts_func, @function
enable_interrupts_func:
    sti
    ret

.globl disable_interrupts_func
.type disable_interrupts_func, @function
disable_interrupts_func:
    cli
    ret

.globl interrupts_enabled_func
.type interrupts_enabled_func, @function
interrupts_enabled_func:
    pushfq
    pop %rax
    and $0x200, %rax
    shr $9, %rax
    ret

/*============================================================================
 * SET HANDLER POINTERS
 *============================================================================*/

.globl set_exception_handler_ptr
.type set_exception_handler_ptr, @function
set_exception_handler_ptr:
    movq %rdi, exception_handler_ptr(%rip)
    ret

.globl set_interrupt_handler_ptr
.type set_interrupt_handler_ptr, @function
set_interrupt_handler_ptr:
    movq %rdi, interrupt_handler_ptr(%rip)
    ret

.globl set_eoi_handler_ptr
.type set_eoi_handler_ptr, @function
set_eoi_handler_ptr:
    movq %rdi, eoi_handler_ptr(%rip)
    ret

.globl set_should_schedule_ptr
.type set_should_schedule_ptr, @function
set_should_schedule_ptr:
    movq %rdi, should_schedule_ptr(%rip)
    ret

.globl set_syscall_handler_ptr
.type set_syscall_handler_ptr, @function
set_syscall_handler_ptr:
    movq %rdi, sycall_handler_ptr(%rip)
    ret

.globl set_page_fault_handler_ptr
.type set_page_fault_handler_ptr, @function
set_page_fault_handler_ptr:
    movq %rdi, page_fault_c_handler(%rip)
    ret
