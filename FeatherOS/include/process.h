/* FeatherOS - Process Management
 * Sprint 9: Process Structure & Creation
 */

#ifndef FEATHEROS_PROCESS_H
#define FEATHEROS_PROCESS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Process ID type */
typedef int32_t pid_t;
#define PID_MAX 32768
#define PID_MAX_DEFAULT 32768

/*============================================================================
 * PROCESS CONSTANTS
 *============================================================================*/

#define MAX_PROCESSES 64
#define KERNEL_STACK_SIZE 16384
#define USER_STACK_SIZE (1024 * 1024)

/*============================================================================
 * TASK FLAGS
 *============================================================================*/

#define TF_RUNNING      0x01
#define TF_INTERRUPT    0x02
#define TF_BLOCKED      0x04
#define TF_ZOMBIE       0x08
#define TF_DETACHED     0x10
#define TF_KTHREAD      0x20  /* Kernel thread */
#define TF_IDLE         0x40  /* Idle task */

/*============================================================================
 * PROCESS STATE
 *============================================================================*/

/* FPU state size for x87/SSE */
#define FPU_STATE_SIZE 512

typedef enum {
    TASK_STATE_NEW = 0,
    TASK_STATE_READY,
    TASK_STATE_RUNNING,
    TASK_STATE_BLOCKED,
    TASK_STATE_SLEEPING,
    TASK_STATE_STOPPED,
    TASK_STATE_ZOMBIE,
    TASK_STATE_EXIT
} task_state_t;

/*============================================================================
 * CPU CONTEXT (for context switching)
 *============================================================================*/

typedef struct {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rsp;
    uint64_t rflags;
    uint64_t cs, ss, ds, es, fs, gs;
} cpu_context_t;

/*============================================================================
 * PROCESS/THREAD STRUCTURE (task_struct)
 *============================================================================*/

typedef struct task_struct {
    /* Process identification */
    pid_t pid;                    /* Process ID */
    pid_t ppid;                   /* Parent PID */
    pid_t pgid;                   /* Process group ID */
    pid_t sid;                    /* Session ID */
    uint32_t uid;                 /* User ID */
    uint32_t gid;                 /* Group ID */
    
    /* Task name */
    const char *name;
    
    /* Task state */
    volatile task_state_t state;
    uint32_t flags;
    int exit_code;
    
    /* CPU context */
    cpu_context_t cpu_context;
    uint64_t kernel_stack;
    uint64_t user_stack;
    
    /* Memory */
    uint64_t cr3;                 /* Page table base */
    uint64_t start_brk;          /* Start of heap */
    uint64_t brk;                /* Current brk */
    uint64_t start_code;
    uint64_t end_code;
    uint64_t start_data;
    uint64_t end_data;
    
    /* FPU/SSE State */
    uint8_t fpu_state[FPU_STATE_SIZE];  /* FPU register state */
    
    /* Scheduling */
    int priority;                 /* Nice value: -20 to +19 */
    int static_priority;
    int dynamic_priority;
    uint64_t vruntime;           /* Virtual runtime for CFS */
    uint64_t sum_exec_runtime;    /* Total execution time */
    uint64_t last_sched;          /* Last scheduled time */
    uint64_t sched_latency;
    uint64_t slice_runtime;
    
    /* CFS-specific fields */
    struct {
        void *my_q;              /* Run queue task belongs to (void* to avoid circular) */
        struct task_struct *parent;   /* Parent in CFS tree */
        struct task_struct *left;    /* Left child in CFS tree */
        struct task_struct *right;   /* Right child in CFS tree */
        uint64_t load_weight;        /* Load weight based on nice */
        uint64_t exec_start;         /* Execution start time */
    } cfs;
    
    /* Sleep deadline (for schedule_timeout) */
    uint64_t sleep_deadline;
    
    /* Timing */
    uint64_t utime;              /* User time */
    uint64_t stime;              /* Kernel time */
    uint64_t start_time;         /* Wall time when started */
    uint64_t real_start_time;    /* Boot time when started */
    
    /* Signal handling */
    uint64_t pending_signals;
    uint64_t blocked_signals;
    uint64_t signal_mask;
    void *sighand;
    
    /* File descriptors */
    int fd_table[32];
    int num_fds;
    
    /* Parent/child relationship */
    struct task_struct *parent;
    struct task_struct *children;
    struct task_struct *sibling;
    
    /* List linkage */
    struct task_struct *next;
    struct task_struct *prev;
    
    /* Wait queue */
    struct wait_queue *wait_queue;
    
    /* Thread-local storage */
    void *tls_area;
    
    /* CPU affinity */
    uint64_t cpu_mask;
    int cpu;
    
    /* Thread group leader */
    struct task_struct *group_leader;
    
    /* Ptrace */
    uint32_t ptrace_flags;
    pid_t tracer_pid;
    
    /* Process credentials */
    uint32_t cap_effective;
    uint32_t cap_permitted;
    uint32_t cap_inheritable;
    
    /* Kernel thread function */
    void (*kernel_thread_fn)(void *arg);
    void *kernel_thread_arg;
} task_t;

/*============================================================================
 * WAIT QUEUE
 *============================================================================*/

typedef struct wait_queue {
    task_t *task;
    struct wait_queue *next;
} wait_queue_t;

/*============================================================================
 * PID ALLOCATOR
 *============================================================================*/

typedef struct {
    pid_t next_pid;
    uint64_t bitmap[(MAX_PROCESSES + 63) / 64];
} pid_allocator_t;

/*============================================================================
 * PROCESS STATISTICS
 *============================================================================*/

typedef struct {
    uint32_t total_tasks;
    uint32_t running_tasks;
    uint32_t blocked_tasks;
    uint32_t zombie_tasks;
    uint32_t kernel_threads;
    uint32_t user_tasks;
    pid_t last_pid;
    pid_t max_pid;
} process_stats_t;

/*============================================================================
 * PROCESS TABLE
 *============================================================================*/

typedef struct {
    task_t *tasks[MAX_PROCESSES];
    uint32_t num_tasks;
    task_t *idle_task;
    task_t *init_task;
    task_t *current;
    task_t *previous;
    pid_allocator_t pid_alloc;
    process_stats_t stats;
    bool initialized;
} process_table_t;

/*============================================================================
 * FUNCTION DECLARATIONS
 *============================================================================*/

/* Initialize process subsystem */
int process_init(void);

/* Get current task */
task_t *get_current_task(void);

/* Get process table */
process_table_t *get_process_table(void);

/* PID allocator */
pid_t alloc_pid(void);
void free_pid(pid_t pid);
bool is_pid_valid(pid_t pid);

/* Task creation */
task_t *kernel_thread(void (*fn)(void *), void *arg, int flags);
task_t *copy_process(unsigned long clone_flags, unsigned long stack_start);
int do_exit(int exit_code);
int sys_exit(int exit_code);
pid_t sys_exit_group(int exit_code);

/* Task lifecycle */
int wake_up_process(task_t *task);
int try_to_wake_up(task_t *task);
void schedule(void);
void yield(void);
void schedule_tail(task_t *prev);

/* Task state management */
void set_task_state(task_t *task, task_state_t state);
task_state_t get_task_state(task_t *task);
void set_task_flags(task_t *task, uint32_t flags);
void clear_task_flags(task_t *task, uint32_t flags);

/* Wait queue */
void init_wait_queue(wait_queue_t *wq);
void add_to_wait_queue(wait_queue_t *wq, task_t *task);
void remove_from_wait_queue(wait_queue_t *wq, task_t *task);
void wake_up(wait_queue_t *wq);

/* Process table operations */
task_t *find_task_by_pid(pid_t pid);
task_t *find_task_by_name(const char *name);
int add_task(task_t *task);
int remove_task(task_t *task);

/* Process statistics */
process_stats_t *get_process_stats(void);
void print_process_list(void);
void print_task_info(task_t *task);

/* Idle task */
void idle_task(void *arg) __attribute__((noreturn));

/* Scheduler hooks */
void sched_init(void);
void sched_tick(void);
void sched_fork(task_t *task);
void sched_exit(task_t *task);

/* kernel_thread helper */
int kernel_thread_helper(void);

/* Context switch statistics */
void context_switch_print_stats(void);
uint64_t context_switch_get_count(void);
uint64_t context_switch_get_avg_time(void);

#endif /* FEATHEROS_PROCESS_H */
