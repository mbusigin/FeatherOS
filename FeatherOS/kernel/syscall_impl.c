/*
 * FeatherOS Syscall Handler - C Part
 */

#include <syscall.h>
#include <kernel.h>

/* External reference to syscall handler pointer setter in interrupt.S */
extern void set_syscall_handler_ptr(unsigned long addr);

/* Syscall handler type - takes 6 args like x86_64 syscall */
typedef unsigned long (*syscall_handler_t)(unsigned long, unsigned long, unsigned long, 
                                          unsigned long, unsigned long, unsigned long);

/* Syscall handlers */
static unsigned long sys_read(unsigned long fd, unsigned long buf, unsigned long count,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_write(unsigned long fd, unsigned long buf, unsigned long count,
                               unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_exit(unsigned long code, unsigned long arg2, unsigned long arg3,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_brk(unsigned long addr, unsigned long arg2, unsigned long arg3,
                             unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_sched_yield(unsigned long arg1, unsigned long arg2, unsigned long arg3,
                                      unsigned long arg4, unsigned long arg5, unsigned long arg6);

/* Syscall table - indexed by syscall number */
static syscall_handler_t syscall_table[64] = {
    [0] = sys_read,
    [1] = sys_write,
    [12] = sys_brk,
    [24] = sys_sched_yield,
    [60] = sys_exit,
};

/*
 * Main syscall handler - called from assembly via syscall_entry
 * rdi = syscall number
 * rsi, rdx, r10, r8, r9 = arguments
 * Returns: result in rax
 */
unsigned long do_syscall(unsigned long syscall_num,
                        unsigned long arg1,
                        unsigned long arg2,
                        unsigned long arg3,
                        unsigned long arg4,
                        unsigned long arg5)
{
    unsigned long result = -1;
    
    if (syscall_num >= 64) {
        return -1;
    }
    
    syscall_handler_t handler = syscall_table[syscall_num];
    
    if (handler) {
        result = handler(arg1, arg2, arg3, arg4, arg5, 0);
    } else {
        result = -1;
    }
    
    return result;
}

/* ===== Basic Syscalls ===== */

static unsigned long sys_read(unsigned long fd, unsigned long buf, unsigned long count,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)fd; (void)buf; (void)count; (void)arg4; (void)arg5; (void)arg6;
    /* TODO: actual keyboard/serial read */
    return 0;
}

static unsigned long sys_write(unsigned long fd, unsigned long buf, unsigned long count,
                               unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)fd; (void)buf; (void)count; (void)arg4; (void)arg5; (void)arg6;
    /* TODO: actual output */
    if (fd == 1 || fd == 2) return count;
    return -1;
}

static unsigned long sys_exit(unsigned long code, unsigned long arg2, unsigned long arg3,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)code; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    while (1) __asm__ volatile ("hlt");
    return 0;
}

static unsigned long sys_brk(unsigned long addr, unsigned long arg2, unsigned long arg3,
                             unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    static unsigned long current_brk = 0;
    if (addr == 0) return current_brk;
    current_brk = addr;
    return current_brk;
}

static unsigned long sys_sched_yield(unsigned long arg1, unsigned long arg2, unsigned long arg3,
                                      unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return 0;
}

/*
 * Set up syscall handling - called from kernel init
 */
void syscall_init(void) {
    /* Set the syscall handler pointer used by interrupt.S */
    set_syscall_handler_ptr((unsigned long)do_syscall);
}
