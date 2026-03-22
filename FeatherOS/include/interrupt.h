/* FeatherOS - Interrupt Handling
 * Sprint 13: Interrupt Handling
 */

#ifndef FEATHEROS_INTERRUPT_H
#define FEATHEROS_INTERRUPT_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * INTERRUPT DESCRIPTOR TABLE (IDT)
 *============================================================================*/

#define IDT_SIZE 256
#define IDT_PRESENT (1 << 15)
#define IDT_USER (3 << 13)
#define IDT_INTERRUPT_GATE 0xE
#define IDT_TRAP_GATE 0xF

/* IDT entry structure */
typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;          /* Interrupt Stack Table */
    uint8_t flags;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry_t;

/* IDT pointer */
typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_pointer_t;

/*============================================================================
 * INTERRUPT FRAME
 *============================================================================*/

/* Pushed by processor on interrupt */
typedef struct {
    uint64_t rdi, rsi, rbp, rbx, rdx, rcx, rax;  /* General purpose */
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;
    uint64_t error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} interrupt_frame_t;

/*============================================================================
 * INTERRUPT CONTROLLER
 *============================================================================*/

/* IRQ numbers */
#define IRQ_TIMER        0
#define IRQ_KEYBOARD     1
#define IRQ_SLAVE_PIC    2
#define IRQ_COM2         3
#define IRQ_COM1         4
#define IRQ_LPT1         5
#define IRQ_FLOPPY       6
#define IRQ_DISKETTE     7
#define IRQ_PS2_MOUSE    12
#define IRQ_FPU          13
#define IRQ_HDD          14
#define IRQ_MATH         13

#define IRQ_BASE         0x20
#define NUM_IRQS         16

/*============================================================================
 * INTERRUPT HANDLER
 *============================================================================*/

typedef void (*interrupt_handler_t)(interrupt_frame_t *frame);

typedef struct {
    interrupt_handler_t handler;
    void *dev;
    const char *name;
} irq_handler_t;

/*============================================================================
 * EXCEPTION HANDLERS
 *============================================================================*/

enum {
    EXCEPTION_DE  = 0,   /* Division Error */
    EXCEPTION_DB  = 1,   /* Debug */
    EXCEPTION_NMI = 2,   /* Non-Maskable Interrupt */
    EXCEPTION_BP  = 3,   /* Breakpoint */
    EXCEPTION_OF  = 4,   /* Overflow */
    EXCEPTION_BR  = 5,   /* Bound Range Exceeded */
    EXCEPTION_UD  = 6,   /* Invalid Opcode */
    EXCEPTION_NM  = 7,   /* Device Not Available */
    EXCEPTION_DF  = 8,   /* Double Fault */
    EXCEPTION_TS  = 10,  /* Invalid TSS */
    EXCEPTION_NP  = 11,  /* Segment Not Present */
    EXCEPTION_SS  = 12,  /* Stack Fault */
    EXCEPTION_GP  = 13,  /* General Protection */
    EXCEPTION_PF  = 14,  /* Page Fault */
    EXCEPTION_MF  = 16,  /* Math Fault */
    EXCEPTION_AC  = 17,  /* Alignment Check */
    EXCEPTION_MC  = 18,  /* Machine Check */
    EXCEPTION_XF  = 19,  /* SIMD Floating Point */
};

/*============================================================================
 * INTERRUPT MANAGEMENT
 *============================================================================*/

/* Initialize IDT and interrupt handling */
void idt_init(void);
void interrupt_init(void);

/* Register exception handler */
void register_exception_handler(int vector, interrupt_handler_t handler);

/* Register IRQ handler */
int register_irq_handler(int irq, interrupt_handler_t handler, const char *name, void *dev);
int unregister_irq_handler(int irq, interrupt_handler_t handler);

/* Enable/disable interrupts */
void enable_interrupts(void);
void disable_interrupts(void);
bool interrupts_enabled(void);

/* End of interrupt */
void eoi(int irq);

/*============================================================================
 * INTERRUPT STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t total_interrupts;
    uint64_t exceptions[32];
    uint64_t irqs[NUM_IRQS];
    uint64_t spurious_irqs;
    uint64_t interrupts_per_cpu[256];
} interrupt_stats_t;

/* Get interrupt statistics */
interrupt_stats_t *get_interrupt_stats(void);

/* Print interrupt statistics */
void print_interrupt_stats(void);

/*============================================================================
 * TASKLETS (Bottom Half Processing)
 *============================================================================*/

typedef struct tasklet_struct {
    void (*func)(unsigned long data);
    unsigned long data;
    int state;
    struct tasklet_struct *next;
} tasklet_t;

#define TASKLET_STATE_SCHED    0
#define TASKLET_STATE_RUN      1

/* Tasklet management */
void tasklet_init(tasklet_t *t, void (*func)(unsigned long), unsigned long data);
void tasklet_schedule(tasklet_t *t);
void tasklet_hi_schedule(tasklet_t *t);
void tasklet_kill(tasklet_t *t);

/* Process pending tasklets */
void tasklet_process(void);

/*============================================================================
 * WORK QUEUES (Bottom Half Processing)
 *============================================================================*/

typedef struct work_struct {
    void (*func)(void *data);
    void *data;
    struct work_struct *next;
} work_t;

/* Work queue */
typedef struct {
    work_t *head;
    work_t *tail;
    volatile int pending;
} workqueue_t;

/* Initialize work queue */
void init_workqueue(workqueue_t *wq);

/* Schedule work */
void schedule_work(work_t *work);
void schedule_delayed_work(work_t *work, uint64_t ticks);

/* Process work queue */
void process_workqueue(workqueue_t *wq);

/*============================================================================
 * SOFTIRQ (Bottom Half for Performance Critical)
 *============================================================================*/

typedef void (*softirq_handler_t)(void);

enum {
    SOFTIRQ_TIMER,
    SOFTIRQ_NET,
    SOFTIRQ_BLOCK,
    SOFTIRQ_SCHED,
    NR_SOFTIRQS
};

/* Raise softirq */
void raise_softirq(int nr);

/* Process softirqs */
void do_softirq(void);

/* Process all pending softirqs */
void softirq_process(void);

/*============================================================================
 * INTERRUPT CONTROLLER ABSTRACTION
 *============================================================================*/

/* PIC (Programmable Interrupt Controller) */
void pic_init(void);
void pic_enable_irq(int irq);
void pic_disable_irq(int irq);
void pic_eoi(int irq);

/* APIC (Advanced PIC) */
void apic_init(void);
void apic_enable(void);
void apic_disable(void);
void apic_eoi(void);
void apic_send_ipi(int cpu, int vector);

#endif /* FEATHEROS_INTERRUPT_H */
