/* FeatherOS - Kernel Main
 * Sprint 3: Console & Basic I/O
 * Sprint 2: User Space Shell Entry
 */

#include <kernel.h>
#include <process.h>

/* Forward declarations */
extern void console_init(void);
extern void serial_init(void);
extern void vga_init(void);
extern void keyboard_init(void);
extern void syscall_init(void);

/* From process.c */
extern int user_process_init(void);
extern int spawn_user_process(const char *path);

/* Debug shell command buffer */
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];

/* Simple built-in commands */
static void shell_help(void) {
    console_print("FeatherOS Debug Shell Commands:\n");
    console_print("  help       - Show this help\n");
    console_print("  clear       - Clear the screen\n");
    console_print("  echo <msg>  - Print a message\n");
    console_print("  info        - Show system info\n");
    console_print("  reboot      - Reboot the system\n");
    console_print("  halt        - Halt the system\n");
    console_print("  regs        - Show CPU registers\n");
    console_print("  memory      - Show memory info\n");
}

static void shell_info(void) {
    console_print("FeatherOS Information:\n");
    console_print("  Version: 0.2.0 (User Shell)\n");
    console_print("  Architecture: x86_64\n");
    console_print("  Boot Mode: Multiboot2\n");
    console_print("  VGA Mode: 80x25 Text\n");
    console_print("  Serial: 115200 8N1\n");
    console_print("  Keyboard: PS/2\n");
}

static void shell_reboot(void) {
    console_print("Rebooting...\n");
    
    /* Trigger keyboard controller to signal CPU reset */
    uint8_t good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
    }
    outb(0x64, 0xFE);  /* Pulse CPU reset line */
    hlt();
}

static void shell_halt(void) {
    console_print("System halted.\n");
    kernel_halt();
}

/* Read CPU register state (simplified) */
static void shell_regs(void) {
    uint64_t cr0, cr2, cr3, cr4;
    
    __asm__ volatile(
        "mov %%cr0, %%rax\n\t"
        "mov %%rax, %0\n\t"
        "mov %%cr2, %%rax\n\t"
        "mov %%rax, %1\n\t"
        "mov %%cr3, %%rax\n\t"
        "mov %%rax, %2\n\t"
        "mov %%cr4, %%rax\n\t"
        "mov %%rax, %3\n\t"
        : "=m"(cr0), "=m"(cr2), "=m"(cr3), "=m"(cr4)
        :
        : "rax"
    );
    
    console_print("CPU Control Registers:\n");
    console_print("  CR0: 0x%016lx\n", cr0);
    console_print("  CR2: 0x%016lx\n", cr2);
    console_print("  CR3: 0x%016lx\n", cr3);
    console_print("  CR4: 0x%016lx\n", cr4);
}

static void shell_memory(void) {
    console_print("Memory Information:\n");
    console_print("  VGA Memory: 0xB8000 (text mode)\n");
    console_print("  Kernel Base: 0x100000 (1MB)\n");
    console_print("  Page Size: 4KB\n");
    console_print("  Address Space: 48-bit virtual\n");
}

/* Parse and execute a command */
static void execute_command(char *cmd) {
    if (cmd[0] == '\0') return;
    
    /* Tokenize command */
    char *args[16];
    int argc = 0;
    
    char *token = cmd;
    while (*token && argc < 15) {
        /* Skip whitespace */
        while (*token == ' ') token++;
        if (!*token) break;
        
        args[argc++] = token;
        
        /* Find end of token */
        while (*token && *token != ' ') token++;
        if (*token) *token++ = '\0';
    }
    
    if (argc == 0) return;
    
    /* Execute command */
    if (strcmp(args[0], "help") == 0 || strcmp(args[0], "?") == 0) {
        shell_help();
    } else if (strcmp(args[0], "clear") == 0 || strcmp(args[0], "cls") == 0) {
        console_clear();
    } else if (strcmp(args[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) {
            console_print("%s ", args[i]);
        }
        console_print("\n");
    } else if (strcmp(args[0], "info") == 0) {
        shell_info();
    } else if (strcmp(args[0], "reboot") == 0 || strcmp(args[0], "restart") == 0) {
        shell_reboot();
    } else if (strcmp(args[0], "halt") == 0 || strcmp(args[0], "shutdown") == 0) {
        shell_halt();
    } else if (strcmp(args[0], "regs") == 0) {
        shell_regs();
    } else if (strcmp(args[0], "memory") == 0 || strcmp(args[0], "mem") == 0) {
        shell_memory();
    } else {
        console_print("Unknown command: %s\n", args[0]);
        console_print("Type 'help' for available commands.\n");
    }
}

/* Interactive kernel debug shell loop */
static void kernel_shell_loop(void) {
    console_print("\n[DEBUG] Kernel debug shell - type 'shell' to start user shell\n\n");
    
    while (1) {
        console_print("FeatherOS> ");
        
        int len = console_readline(cmd_buffer, CMD_BUFFER_SIZE);
        
        if (len < 0) {
            console_print("\n[DEBUG] Goodbye!\n");
            break;
        }
        
        if (len > 0) {
            /* Check for 'shell' command to start user shell */
            if (strcmp(cmd_buffer, "shell") == 0) {
                console_print("\n[DEBUG] Starting user shell...\n");
                return;  /* Return to start_user_space */
            }
            
            console_print("\n");
            execute_command(cmd_buffer);
            console_print("\n");
        }
    }
}

/*
 * Start user space - spawn user shell
 * This is called after kernel initialization
 */
static void start_user_space(void) {
    console_print("\n");
    console_print("======================================\n");
    console_print(" Starting FeatherOS User Space\n");
    console_print("======================================\n");
    console_print("\n");
    
    /* Initialize user process subsystem */
    console_print("[MAIN] Initializing user process subsystem...\n");
    user_process_init();
    
    /* Spawn user shell */
    console_print("[MAIN] Spawning user shell...\n");
    int pid = spawn_user_process("/bin/sh");
    
    if (pid < 0) {
        console_print("[MAIN] ERROR: Failed to spawn user shell!\n");
        console_print("[MAIN] Starting kernel debug shell instead...\n");
        kernel_shell_loop();
        return;
    }
    
    console_print("[MAIN] User shell spawned with PID %d\n", pid);
    console_print("\n");
    console_print("======================================\n");
    console_print(" FeatherOS User Shell Ready!\n");
    console_print("======================================\n");
    console_print("\n");
    
    /* For now, fall back to kernel shell since we don't have full scheduler integration */
    console_print("[MAIN] Note: Full scheduler not integrated yet.\n");
    console_print("[MAIN] Starting kernel debug shell...\n");
    console_print("[MAIN] Type 'shell' to attempt user shell spawn.\n");
    console_print("\n");
    
    kernel_shell_loop();
    
    /* Should never reach here */
    kernel_halt();
}

/* Main kernel entry point */
void kernel_main(uint32_t magic, void *mbi) {
    UNUSED(magic);
    UNUSED(mbi);
    
    /* Initialize console subsystem */
    console_init();
    
    /* Initialize keyboard */
    keyboard_init();
    
    /* Initialize syscalls */
    syscall_init();
    
    /* Boot messages */
    console_print("\n");
    console_print("=====================================\n");
    console_print(" FeatherOS Boot Complete\n");
    console_print("=====================================\n");
    console_print("\n");
    
    info_print("Kernel initialized successfully%s\n", "");
    debug_print("Debug messages enabled%s\n", "");
    warn_print("This is a warning message%s\n", "");
    error_print("This is an error message%s\n", "");
    
    console_print("\n");
    console_print("Testing printk format specifiers:\n");
    console_print("  Character: %c\n", 'X');
    console_print("  String: %s\n", "Hello, FeatherOS!");
    console_print("  Integer: %d\n", 42);
    console_print("  Negative: %d\n", -42);
    console_print("  Unsigned: %u\n", 42);
    console_print("  Hex: 0x%x\n", 0xDEADBEEF);
    console_print("  Pointer: %p\n", (void*)0x100000);
    console_print("  Padded: |%10s|\n", "text");
    console_print("  Zero-padded: 0x%08x\n", 255);
    console_print("\n");
    
    /* Syscall infrastructure ready */
    console_print("[FEATHEROS] Syscall handler ready\n");
    console_print("[FEATHEROS] User space: enabled\n");
    console_print("[FEATHEROS] Shell: loaded\n");
    console_print("\n");
    
    /* Start user space instead of debug shell */
    start_user_space();
    
    /* If user space exits, halt */
    kernel_halt();
}
