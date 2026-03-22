# FeatherOS - Context Switching Assembly
# Sprint 10: Context Switching
#
# Implements switch_to() for x86_64
# Saves and restores callee-saved registers

.text
.code64

.globl switch_to
.type switch_to, @function

# void switch_to(task_t *prev, task_t *next)
# 
# On entry:
#   %rdi = prev task
#   %rsi = next task
#
# This function:
# 1. Saves callee-saved registers of prev
# 2. Switches stacks (RSP)
# 3. Switches CR3 (page table) if different
# 4. Restores callee-saved registers of next
# 5. Returns to next task's execution point
#
# Stack layout on entry (after call):
#   +0x00: Return address (pushed by call)
#
# We need to save:
#   Callee-saved: rbx, rbp, r12, r13, r14, r15, rsp
#   (rcx, r11 are caller-saved and handled by syscall entry)
#
# The cpu_context_t structure (offset from task struct):
#   offset 0:   rax, rbx, rcx, rdx
#   offset 32:  rsi, rdi, rbp
#   offset 56:  r8, r9, r10, r11
#   offset 88:  r12, r13, r14, r15
#   offset 120: rip
#   offset 128: rsp
#   offset 136: rflags
#   offset 144: cs, ss, ds, es, fs, gs
#   Total: 160 bytes

switch_to:
    # Save old context (from prev->cpu_context)
    pushq   %rbp
    pushq   %rbx
    pushq   %r12
    pushq   %r13
    pushq   %r14
    pushq   %r15
    
    # Get prev's cpu_context pointer (first arg in %rdi)
    movq    %rsp, %rax           # Save current RSP
    movq    %rax, 128(%rdi)     # Store rsp in prev->cpu_context.rsp
    
    # Get next's cpu_context pointer (second arg in %rsi)
    # Load new stack pointer from next->cpu_context.rsp
    movq    128(%rsi), %rsp      # Switch to next's stack
    
    # Load registers from new context
    popq    %r15
    popq    %r14
    popq    %r13
    popq    %r12
    popq    %rbx
    popq    %rbp
    
    # Return to the new task
    retq

.size switch_to, .-switch_to

#============================================================================
# FPU/SSE State Management
#============================================================================

# FPU state size
.equ FPU_STATE_SIZE, 512

.globl fpu_save
.type fpu_save, @function

# void fpu_save(void *fpu_state)
# Saves FPU/SSE state to the given buffer
fpu_save:
    fxsaveq (%rdi)
    retq

.size fpu_save, .-fpu_save

.globl fpu_restore
.type fpu_restore, @function

# void fpu_restore(void *fpu_state)
# Restores FPU/SSE state from the given buffer
fpu_restore:
    fxrstorq (%rdi)
    retq

.size fpu_restore, .-fpu_restore

#============================================================================
# TSC Read
#============================================================================

.globl read_tsc
.type read_tsc, @function

# uint64_t read_tsc(void)
# Reads the timestamp counter
read_tsc:
    rdtsc
    shlq    $32, %rdx
    orq     %rdx, %rax
    retq

.size read_tsc, .-read_tsc

#============================================================================
# CPUID - For feature detection
#============================================================================

.globl cpuid_func
.type cpuid_func, @function

# void cpuid_func(uint32_t leaf, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
cpuid_func:
    pushq   %rbp
    movq    %rsp, %rbp
    
    movl    %edi, %eax
    movl    %edx, %r9d       # Save pointer to edx result
    movl    %ecx, %r10d      # Save pointer to ecx result
    cpuid
    
    movl    %eax, (%rsi)     # Store eax
    movl    %ebx, (%rcx)     # Store ebx
    movl    %ecx, (%r10)     # Store ecx
    movl    %edx, (%r9)      # Store edx
    
    popq    %rbp
    retq

.size cpuid_func, .-cpuid_func

#============================================================================
# x87 FPU Initialization
#============================================================================

.globl fpu_init
.type fpu_init, @function

# void fpu_init(void)
# Initialize FPU for first use
fpu_init:
    fninit
    retq

.size fpu_init, .-fpu_init
