/* FeatherOS - Process Management Implementation
 * Sprint 9: Process Structure & Creation
 */

#include <process.h>
#include <kernel.h>
#include <datastructures.h>
#include <paging.h>
#include <memory.h>

/*============================================================================
 * GLOBALS
 *============================================================================*/

/* Process table */
static process_table_t process_table = {0};

/* Current task pointer */
static task_t *current_task = NULL;

/* Forward declarations */
static void switch_to(task_t *prev, task_t *next);

/*============================================================================
 * PID ALLOCATOR
 *============================================================================*/

int pid_allocator_init(pid_allocator_t *alloc) {
    if (!alloc) return -1;
    memset(alloc, 0, sizeof(pid_allocator_t));
    alloc->next_pid = 1;  /* PID 0 is reserved for idle */
    
    /* Reserve PID 0 */
    alloc->bitmap[0] = 1;  /* Bit 0 set = reserved */
    
    return 0;
}

pid_t alloc_pid(void) {
    pid_allocator_t *alloc = &process_table.pid_alloc;
    pid_t start = alloc->next_pid;
    
    for (int attempt = 0; attempt < MAX_PROCESSES; attempt++) {
        pid_t pid = alloc->next_pid;
        alloc->next_pid++;
        
        if (alloc->next_pid >= MAX_PROCESSES) {
            alloc->next_pid = 1;  /* Wrap around */
        }
        
        /* Check if PID is in use */
        uint32_t index = pid / 64;
        uint32_t bit = pid % 64;
        
        if (!(alloc->bitmap[index] & (1ULL << bit))) {
            /* Mark as used */
            alloc->bitmap[index] |= (1ULL << bit);
            process_table.stats.last_pid = pid;
            return pid;
        }
        
        if (alloc->next_pid == start) {
            /* Full circle, no free PIDs */
            return -1;
        }
    }
    
    return -1;
}

void free_pid(pid_t pid) {
    if (pid <= 0 || pid >= MAX_PROCESSES) return;
    
    pid_allocator_t *alloc = &process_table.pid_alloc;
    uint32_t index = pid / 64;
    uint32_t bit = pid % 64;
    
    alloc->bitmap[index] &= ~(1ULL << bit);
    
    if (pid < alloc->next_pid) {
        alloc->next_pid = pid;
    }
}

bool is_pid_valid(pid_t pid) {
    return pid > 0 && pid < MAX_PROCESSES;
}

/*============================================================================
 * TASK STRUCTURE
 *============================================================================*/

static task_t *allocate_task(void) {
    task_t *task = (task_t*)kmalloc(sizeof(task_t));
    if (!task) return NULL;
    
    memset(task, 0, sizeof(task_t));
    return task;
}

static void free_task(task_t *task) {
    if (!task) return;
    
    /* Free resources */
    if (task->kernel_stack) {
        /* Would free kernel stack */
    }
    
    kfree(task);
}

/*============================================================================
 * TASK CREATION
 *============================================================================*/

task_t *kernel_thread(void (*fn)(void *), void *arg, int flags) {
    task_t *task = allocate_task();
    if (!task) return NULL;
    
    /* Allocate PID */
    task->pid = alloc_pid();
    if (task->pid < 0) {
        free_task(task);
        return NULL;
    }
    
    /* Set up task */
    task->name = "kernel_thread";
    task->state = TASK_STATE_READY;
    task->flags = TF_KTHREAD;
    if (flags) task->flags |= flags;
    
    task->priority = 0;
    task->static_priority = 0;
    task->dynamic_priority = 0;
    
    /* Set kernel thread function */
    task->kernel_thread_fn = fn;
    task->kernel_thread_arg = arg;
    
    /* Set up CPU context for kernel thread */
    task->cpu_context.rip = (uint64_t)fn;
    task->cpu_context.rdi = (uint64_t)arg;
    task->cpu_context.rsp = (uint64_t)task + KERNEL_STACK_SIZE - 8;
    task->cpu_context.rflags = 0x202;  /* IF bit set */
    task->cpu_context.cs = 0x08;      /* Kernel code segment */
    task->cpu_context.ss = 0x10;       /* Kernel data segment */
    
    /* Add to process table */
    if (add_task(task) != 0) {
        free_pid(task->pid);
        free_task(task);
        return NULL;
    }
    
    /* Update stats */
    process_table.stats.total_tasks++;
    process_table.stats.kernel_threads++;
    
    return task;
}

task_t *copy_process(unsigned long clone_flags, unsigned long stack_start) {
    task_t *parent = get_current_task();
    task_t *task = allocate_task();
    if (!task) return NULL;
    
    /* Allocate PID */
    task->pid = alloc_pid();
    if (task->pid < 0) {
        free_task(task);
        return NULL;
    }
    
    /* Copy parent info */
    task->ppid = parent ? parent->pid : 0;
    task->uid = parent ? parent->uid : 0;
    task->gid = parent ? parent->gid : 0;
    task->name = parent ? parent->name : "copy_process";
    
    task->state = TASK_STATE_READY;
    task->flags = 0;
    
    /* Copy CPU context */
    if (parent) {
        memcpy(&task->cpu_context, &parent->cpu_context, sizeof(cpu_context_t));
    }
    
    /* Set instruction pointer for return from fork */
    task->cpu_context.rax = 0;  /* Return 0 in child */
    task->cpu_context.rsp = stack_start;
    
    /* Copy file descriptors if not CLONE_FILES */
    if (!(clone_flags & 0x04) && parent) {
        memcpy(task->fd_table, parent->fd_table, sizeof(parent->fd_table));
        task->num_fds = parent->num_fds;
    }
    
    /* Set up parent/child relationship */
    task->parent = parent;
    
    /* Add to process table */
    if (add_task(task) != 0) {
        free_pid(task->pid);
        free_task(task);
        return NULL;
    }
    
    /* Update stats */
    process_table.stats.total_tasks++;
    process_table.stats.user_tasks++;
    
    return task;
}

/*============================================================================
 * TASK STATE MANAGEMENT
 *============================================================================*/

void set_task_state(task_t *task, task_state_t state) {
    if (!task) return;
    task->state = state;
}

task_state_t get_task_state(task_t *task) {
    return task ? task->state : TASK_STATE_ZOMBIE;
}

void set_task_flags(task_t *task, uint32_t flags) {
    if (!task) return;
    task->flags |= flags;
}

void clear_task_flags(task_t *task, uint32_t flags) {
    if (!task) return;
    task->flags &= ~flags;
}

/*============================================================================
 * PROCESS TABLE OPERATIONS
 *============================================================================*/

task_t *find_task_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        task_t *task = process_table.tasks[i];
        if (task && task->pid == pid) {
            return task;
        }
    }
    return NULL;
}

task_t *find_task_by_name(const char *name) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        task_t *task = process_table.tasks[i];
        if (task && task->name && strcmp(task->name, name) == 0) {
            return task;
        }
    }
    return NULL;
}

int add_task(task_t *task) {
    if (!task) return -1;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table.tasks[i] == NULL) {
            process_table.tasks[i] = task;
            process_table.num_tasks++;
            return 0;
        }
    }
    
    return -1;  /* No space */
}

int remove_task(task_t *task) {
    if (!task) return -1;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table.tasks[i] == task) {
            process_table.tasks[i] = NULL;
            process_table.num_tasks--;
            free_pid(task->pid);
            return 0;
        }
    }
    
    return -1;  /* Not found */
}

/*============================================================================
 * SCHEDULING
 *============================================================================*/

void schedule(void) {
    task_t *prev = current_task;
    task_t *next = NULL;
    
    /* Simple round-robin for now */
    int current_index = -1;
    if (prev) {
        for (int i = 0; i < MAX_PROCESSES; i++) {
            if (process_table.tasks[i] == prev) {
                current_index = i;
                break;
            }
        }
    }
    
    /* Find next ready task */
    for (int i = 1; i < MAX_PROCESSES; i++) {
        int index = (current_index + i) % MAX_PROCESSES;
        task_t *task = process_table.tasks[index];
        if (task && (task->state == TASK_STATE_READY || task->state == TASK_STATE_RUNNING)) {
            next = task;
            break;
        }
    }
    
    /* If no user task, use idle */
    if (!next) {
        next = process_table.idle_task;
    }
    
    if (next == prev) return;
    
    current_task = next;
    next->state = TASK_STATE_RUNNING;
    
    if (prev) {
        schedule_tail(prev);
    }
    
    switch_to(prev, next);
}

void schedule_tail(task_t *prev) {
    (void)prev;
    /* Any cleanup after context switch */
}

void yield(void) {
    task_t *current = get_current_task();
    if (current) {
        current->state = TASK_STATE_READY;
    }
    schedule();
}

/*============================================================================
 * PROCESS EXIT
 *============================================================================*/

int do_exit(int exit_code) {
    task_t *current = get_current_task();
    if (!current) return -1;
    
    current->exit_code = exit_code;
    current->state = TASK_STATE_ZOMBIE;
    current->flags |= TF_ZOMBIE;
    
    /* Update stats */
    process_table.stats.running_tasks--;
    process_table.stats.zombie_tasks++;
    
    /* Wake up parent if waiting */
    /* For now, just remove from table */
    remove_task(current);
    
    /* Schedule next task */
    schedule();
    
    /* Should never return */
    while (1) {}
}

int sys_exit(int exit_code) {
    return do_exit(exit_code);
}

pid_t sys_exit_group(int exit_code) {
    (void)exit_code;
    /* Would exit all threads in group */
    return do_exit(exit_code);
}

/*============================================================================
 * WAKE UP
 *============================================================================*/

int wake_up_process(task_t *task) {
    if (!task) return -1;
    
    if (task->state == TASK_STATE_BLOCKED || task->state == TASK_STATE_SLEEPING) {
        task->state = TASK_STATE_READY;
        return 0;
    }
    
    return -1;
}

int try_to_wake_up(task_t *task) {
    return wake_up_process(task);
}

/*============================================================================
 * WAIT QUEUE
 *============================================================================*/

void init_wait_queue(wait_queue_t *wq) {
    if (wq) {
        wq->task = NULL;
        wq->next = NULL;
    }
}

void add_to_wait_queue(wait_queue_t *wq, task_t *task) {
    if (!wq || !task) return;
    
    wait_queue_t *entry = (wait_queue_t*)kmalloc(sizeof(wait_queue_t));
    if (!entry) return;
    
    entry->task = task;
    entry->next = wq->next;
    wq->next = entry;
    
    task->state = TASK_STATE_BLOCKED;
    task->wait_queue = wq;
}

void wake_up(wait_queue_t *wq) {
    if (!wq) return;
    
    wait_queue_t **curr = &wq->next;
    while (*curr) {
        wait_queue_t *entry = *curr;
        task_t *task = entry->task;
        
        if (task && (task->state == TASK_STATE_BLOCKED || task->state == TASK_STATE_SLEEPING)) {
            task->state = TASK_STATE_READY;
            *curr = entry->next;
            kfree(entry);
        } else {
            curr = &entry->next;
        }
    }
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

process_stats_t *get_process_stats(void) {
    /* Update stats */
    process_table.stats.total_tasks = process_table.num_tasks;
    process_table.stats.running_tasks = 0;
    process_table.stats.blocked_tasks = 0;
    process_table.stats.zombie_tasks = 0;
    process_table.stats.kernel_threads = 0;
    process_table.stats.user_tasks = 0;
    process_table.stats.max_pid = MAX_PROCESSES;
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        task_t *task = process_table.tasks[i];
        if (task) {
            switch (task->state) {
                case TASK_STATE_RUNNING:
                case TASK_STATE_READY:
                    process_table.stats.running_tasks++;
                    break;
                case TASK_STATE_BLOCKED:
                case TASK_STATE_SLEEPING:
                    process_table.stats.blocked_tasks++;
                    break;
                case TASK_STATE_ZOMBIE:
                    process_table.stats.zombie_tasks++;
                    break;
                default:
                    break;
            }
            
            if (task->flags & TF_KTHREAD) {
                process_table.stats.kernel_threads++;
            } else {
                process_table.stats.user_tasks++;
            }
        }
    }
    
    return &process_table.stats;
}

void print_process_list(void) {
    console_print("PID  PPID  STATE      NAME                FLAGS\n");
    console_print("---- ----- ---------- ------------------ ------\n");
    
    for (int i = 0; i < MAX_PROCESSES; i++) {
        task_t *task = process_table.tasks[i];
        if (!task) continue;
        
        console_print("%4d ", task->pid);
        console_print("%4d ", task->ppid);
        
        const char *state_str = "UNKNOWN";
        switch (task->state) {
            case TASK_STATE_NEW: state_str = "NEW"; break;
            case TASK_STATE_READY: state_str = "READY"; break;
            case TASK_STATE_RUNNING: state_str = "RUNNING"; break;
            case TASK_STATE_BLOCKED: state_str = "BLOCKED"; break;
            case TASK_STATE_SLEEPING: state_str = "SLEEPING"; break;
            case TASK_STATE_STOPPED: state_str = "STOPPED"; break;
            case TASK_STATE_ZOMBIE: state_str = "ZOMBIE"; break;
            case TASK_STATE_EXIT: state_str = "EXIT"; break;
        }
        console_print("%-8s", state_str);
        
        const char *name = task->name ? task->name : "(null)";
        console_print(" %-18s", name);
        
        console_print(" 0x%02x", task->flags);
        console_print("\n");
    }
    
    console_print("\n");
    process_stats_t *stats = get_process_stats();
    console_print("Total: %d, Running: %d, Blocked: %d, Zombie: %d\n",
        stats->total_tasks, stats->running_tasks, stats->blocked_tasks, stats->zombie_tasks);
}

void print_task_info(task_t *task) {
    if (!task) {
        console_print("Task not found\n");
        return;
    }
    
    console_print("Task: %s (PID: %d)\n", task->name ? task->name : "(null)", task->pid);
    console_print("  Parent PID: %d\n", task->ppid);
    console_print("  State: %d\n", task->state);
    console_print("  Flags: 0x%08x\n", task->flags);
    console_print("  Priority: %d\n", task->priority);
    console_print("  Kernel stack: 0x%lx\n", task->kernel_stack);
    console_print("  User stack: 0x%lx\n", task->user_stack);
    console_print("  CPU context RIP: 0x%lx\n", task->cpu_context.rip);
    console_print("  CPU context RSP: 0x%lx\n", task->cpu_context.rsp);
}

/*============================================================================
 * IDLE TASK
 *============================================================================*/

void idle_task(void *arg) {
    (void)arg;
    
    while (1) {
        /* Halt CPU until interrupt */
        __asm__ volatile("hlt");
        
        /* Yield to other tasks */
        yield();
    }
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int process_init(void) {
    if (process_table.initialized) return 0;
    
    /* Initialize PID allocator */
    pid_allocator_init(&process_table.pid_alloc);
    
    /* Create idle task */
    task_t *idle = allocate_task();
    if (!idle) return -1;
    
    idle->pid = 0;  /* Reserved for idle */
    idle->name = "idle";
    idle->state = TASK_STATE_RUNNING;
    idle->flags = TF_IDLE | TF_RUNNING;
    
    /* Idle task uses current stack */
    idle->kernel_stack = 0;
    
    process_table.idle_task = idle;
    process_table.current = idle;
    current_task = idle;
    
    /* Add to table */
    process_table.tasks[0] = idle;
    process_table.num_tasks = 1;
    
    /* Update stats */
    process_table.stats.total_tasks = 1;
    process_table.stats.kernel_threads = 1;
    
    process_table.initialized = true;
    
    console_print("Process manager initialized\n");
    console_print("  Idle task created (PID 0)\n");
    console_print("  PID allocator: functional\n");
    
    return 0;
}

task_t *get_current_task(void) {
    return current_task;
}

process_table_t *get_process_table(void) {
    return &process_table;
}

/*============================================================================
 * SCHEDULER HOOKS
 *============================================================================*/

void sched_init(void) {
    /* Scheduler initialization */
}

void sched_tick(void) {
    /* Called on each timer tick */
    task_t *current = get_current_task();
    if (current) {
        current->utime++;  /* Increment user time */
    }
}

void sched_fork(task_t *task) {
    (void)task;
    /* Fork-specific scheduling setup */
}

void sched_exit(task_t *task) {
    (void)task;
    /* Exit-specific cleanup */
}

/* kernel_thread helper for starting kernel threads */
int kernel_thread_helper(void) {
    task_t *current = get_current_task();
    if (current && current->kernel_thread_fn) {
        current->kernel_thread_fn(current->kernel_thread_arg);
    }
    do_exit(0);
    return 0;  /* Never reached */
}

/*============================================================================
 * CONTEXT SWITCH
 *============================================================================*/

/* Context switch between tasks - assembly version would be better */
void switch_to(task_t *prev, task_t *next) {
    (void)prev;
    (void)next;
    /* Full context switch would save/restore all registers */
    /* For now, just update current task pointer */
    current_task = next;
    
    /* Update process table current pointer */
    process_table.current = next;
    process_table.previous = prev;
}
