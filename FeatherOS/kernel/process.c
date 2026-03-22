/*
 * FeatherOS - User Process Management
 * Sprint 1: Create process.c for user-space shell support
 */

#include <process.h>
#include <kernel.h>
#include <paging.h>
#include <memory.h>
#include <stdint.h>
#include <string.h>

/*============================================================================
 * EXTERNAL FUNCTIONS
 *============================================================================*/

/* From exec.c */
extern int elf_load(void *elf_data, size_t size, uint64_t *out_entry, uint64_t *out_stack);
extern int flat_binary_load(void *data, size_t size, uint64_t *out_entry, uint64_t *out_stack);

/* From task.c */
extern task_t *allocate_task(void);
extern pid_t alloc_pid(void);

/*============================================================================
 * USER ADDRESS SPACE LAYOUT
 *============================================================================*/

/* User space layout constants */
#define USER_CODE_VADDR      0x400000   /* User code base address */
#define USER_STACK_VADDR     0x7FFFFFFF000   /* User stack */
#define USER_STACK_SIZE      0x200000   /* 2 MB stack */
#define USER_HEAP_VADDR      0x600000   /* User heap start */

/*============================================================================
 * USER PROCESS TABLE
 *============================================================================*/

#define MAX_USER_PROCESSES   32

static struct {
    task_t *processes[MAX_USER_PROCESSES];
    int count;
} user_process_table = {0};

/*============================================================================
 * USER PAGE TABLE MANAGEMENT
 *============================================================================*/

static int setup_user_page_tables_for_process(task_t *task) {
    /* Get current CR3 as base - for now, share kernel page tables */
    uint64_t cr3;
    __asm__ volatile ("mov %%cr3, %%rax" : "=a"(cr3));
    task->cr3 = cr3;
    
    console_print("[PROCESS] Page tables set up (CR3=0x%lx)\n", cr3);
    return 0;
}

/*============================================================================
 * USER PROCESS CREATION
 *============================================================================*/

static int setup_user_address_space(task_t *task) {
    if (setup_user_page_tables_for_process(task) != 0) {
        console_print("[PROCESS] Failed to set up page tables\n");
        return -1;
    }
    
    task->start_code = USER_CODE_VADDR;
    task->end_code = USER_CODE_VADDR + 0x100000;
    task->start_data = USER_CODE_VADDR + 0x100000;
    task->end_data = task->start_data + 0x100000;
    task->start_brk = USER_HEAP_VADDR;
    task->brk = USER_HEAP_VADDR;
    task->user_stack = USER_STACK_VADDR + USER_STACK_SIZE - 8;
    
    console_print("[PROCESS] User address space set up\n");
    console_print("  Code:   0x%lx - 0x%lx\n", task->start_code, task->end_code);
    console_print("  Stack:  0x%lx\n", task->user_stack);
    console_print("  Heap:   0x%lx\n", task->start_brk);
    
    return 0;
}

static int load_binary_into_memory(task_t *task, void *binary_data, size_t size) {
    uint64_t entry_point;
    uint64_t stack_ptr;
    
    int ret = flat_binary_load(binary_data, size, &entry_point, &stack_ptr);
    if (ret != 0) {
        ret = elf_load(binary_data, size, &entry_point, &stack_ptr);
    }
    
    if (ret != 0) {
        console_print("[PROCESS] Failed to load binary (error %d)\n", ret);
        return -1;
    }
    
    task->cpu_context.rip = entry_point;
    task->cpu_context.rsp = stack_ptr;
    
    console_print("[PROCESS] Binary loaded (Entry: 0x%lx, Stack: 0x%lx)\n", 
                  entry_point, stack_ptr);
    return 0;
}

static task_t *create_user_process_from_binary(void *binary_data, size_t size, const char *name) {
    task_t *task = allocate_task();
    if (!task) {
        console_print("[PROCESS] Failed to allocate task\n");
        return NULL;
    }
    
    memset(task, 0, sizeof(task_t));
    
    task->pid = alloc_pid();
    if (task->pid < 0) {
        console_print("[PROCESS] Failed to allocate PID\n");
        kfree(task);
        return NULL;
    }
    
    task->name = name;
    task->state = TASK_STATE_READY;
    task->flags = TF_RUNNING;
    
    if (setup_user_address_space(task) != 0) {
        free_pid(task->pid);
        kfree(task);
        return NULL;
    }
    
    if (load_binary_into_memory(task, binary_data, size) != 0) {
        free_pid(task->pid);
        kfree(task);
        return NULL;
    }
    
    /* Set up CPU context for user mode */
    task->cpu_context.rflags = 0x202;
    task->cpu_context.cs = 0x18;
    task->cpu_context.ss = 0x20;
    task->cpu_context.ds = 0x20;
    task->cpu_context.es = 0x20;
    task->cpu_context.fs = 0x20;
    task->cpu_context.gs = 0x20;
    
    /* Clear registers */
    task->cpu_context.rax = 0;
    task->cpu_context.rbx = 0;
    task->cpu_context.rcx = 0;
    task->cpu_context.rdx = 0;
    task->cpu_context.rsi = 0;
    task->cpu_context.rdi = 0;
    task->cpu_context.rbp = 0;
    task->cpu_context.r8 = 0;
    task->cpu_context.r9 = 0;
    task->cpu_context.r10 = 0;
    task->cpu_context.r11 = 0;
    task->cpu_context.r12 = 0;
    task->cpu_context.r13 = 0;
    task->cpu_context.r14 = 0;
    task->cpu_context.r15 = 0;
    
    /* Add to user process table */
    for (int i = 0; i < MAX_USER_PROCESSES; i++) {
        if (user_process_table.processes[i] == NULL) {
            user_process_table.processes[i] = task;
            user_process_table.count++;
            break;
        }
    }
    
    console_print("[PROCESS] User process created: %s (PID %d)\n", name, task->pid);
    return task;
}

/*============================================================================
 * PUBLIC API
 *============================================================================*/

int spawn_user_process(const char *path) {
    (void)path;
    
    console_print("[PROCESS] Spawning user process: %s\n", path);
    
    void *shell_binary = (void *)0x100000;
    size_t shell_size = 0x10000;
    
    task_t *task = create_user_process_from_binary(shell_binary, shell_size, "shell");
    if (!task) {
        console_print("[PROCESS] Failed to spawn process\n");
        return -1;
    }
    
    console_print("[PROCESS] Process spawned with PID %d\n", task->pid);
    return task->pid;
}

int load_binary(const char *path) {
    (void)path;
    console_print("[PROCESS] load_binary called (stub)\n");
    return 0;
}

int setup_user_address_space_for_current(void) {
    task_t *task = get_current_task();
    if (!task) return -1;
    return setup_user_address_space(task);
}

task_t *get_current_user_process(void) {
    return get_current_task();
}

void print_user_processes(void) {
    console_print("User Processes:\n");
    console_print("  Count: %d\n", user_process_table.count);
    for (int i = 0; i < MAX_USER_PROCESSES; i++) {
        task_t *task = user_process_table.processes[i];
        if (task) {
            console_print("  [%d] %s (PID %d) - State: %d\n",
                       i, task->name, task->pid, task->state);
        }
    }
}

void exit_user_process(int exit_code) {
    task_t *task = get_current_task();
    if (!task) return;
    
    console_print("[PROCESS] Process %s (PID %d) exiting with code %d\n",
                task->name, task->pid, exit_code);
    
    task->exit_code = exit_code;
    task->state = TASK_STATE_ZOMBIE;
    task->flags |= TF_ZOMBIE;
    
    for (int i = 0; i < MAX_USER_PROCESSES; i++) {
        if (user_process_table.processes[i] == task) {
            user_process_table.processes[i] = NULL;
            user_process_table.count--;
            break;
        }
    }
    
    schedule();
    
    while (1) {
        __asm__ volatile ("hlt");
    }
}

int user_process_init(void) {
    console_print("[PROCESS] Initializing user process subsystem\n");
    memset(&user_process_table, 0, sizeof(user_process_table));
    console_print("[PROCESS] User process subsystem initialized\n");
    console_print("  Max user processes: %d\n", MAX_USER_PROCESSES);
    return 0;
}

void dump_process_info(task_t *task) {
    if (!task) {
        console_print("Task is NULL\n");
        return;
    }
    
    console_print("=== Process Info: %s (PID %d) ===\n", task->name, task->pid);
    console_print("  State:    %d\n", task->state);
    console_print("  Flags:    0x%x\n", task->flags);
    console_print("  Exit:     %d\n", task->exit_code);
    console_print("  Code:     0x%lx - 0x%lx\n", task->start_code, task->end_code);
    console_print("  Stack:    0x%lx\n", task->user_stack);
    console_print("  CR3:      0x%lx\n", task->cr3);
    console_print("  RIP:      0x%lx\n", task->cpu_context.rip);
    console_print("  RSP:      0x%lx\n", task->cpu_context.rsp);
}
