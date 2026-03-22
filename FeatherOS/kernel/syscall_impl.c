/*
 * FeatherOS Syscall Handler - Working Implementation
 * Connects userland programs to keyboard and console
 */

#include <stdint.h>
#include <stddef.h>

/* External references */
extern void set_syscall_handler_ptr(unsigned long addr);
extern void console_putchar(char c);
extern char serial_getchar(void);
extern void serial_putchar(char c);

/* Keyboard buffer for stdin */
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static volatile int keyboard_head = 0;  /* Next char to read */
static volatile int keyboard_tail = 0; /* Next slot to write */
static volatile int keyboard_count = 0;

/* Add a character to keyboard buffer (called from keyboard ISR) */
void keyboard_buffer_put(char c) {
    if (keyboard_count < KEYBOARD_BUFFER_SIZE) {
        keyboard_buffer[keyboard_tail] = c;
        keyboard_tail = (keyboard_tail + 1) % KEYBOARD_BUFFER_SIZE;
        keyboard_count++;
    }
}

/* Get a character from keyboard buffer (for sys_read) */
static int keyboard_buffer_get(void) {
    if (keyboard_count > 0) {
        char c = keyboard_buffer[keyboard_head];
        keyboard_head = (keyboard_head + 1) % KEYBOARD_BUFFER_SIZE;
        keyboard_count--;
        return (unsigned char)c;
    }
    return -1;  /* No data available */
}

/* Check if keyboard has data */
static int keyboard_buffer_available(void) {
    return keyboard_count > 0;
}

/* Console output - write character to both VGA and serial */
static void console_write_char(char c) {
    console_putchar(c);
    serial_putchar(c);
}

/* Console output - write string */
static void console_write_string(const char* s, size_t len) {
    for (size_t i = 0; i < len && s[i]; i++) {
        console_write_char(s[i]);
    }
}

/* Syscall handler type */
typedef unsigned long (*syscall_handler_t)(unsigned long, unsigned long, unsigned long, 
                                          unsigned long, unsigned long, unsigned long);

/* Forward declarations */
static unsigned long sys_read(unsigned long fd, unsigned long buf, unsigned long count,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_write(unsigned long fd, unsigned long buf, unsigned long count,
                               unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_open(unsigned long pathname, unsigned long flags, unsigned long mode);
static unsigned long sys_close(unsigned long fd);
static unsigned long sys_execve(unsigned long filename, unsigned long argv, unsigned long envp);
static unsigned long sys_exit(unsigned long code, unsigned long arg2, unsigned long arg3,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_getpid(unsigned long arg1, unsigned long arg2, unsigned long arg3,
                                unsigned long arg4, unsigned long arg5, unsigned long arg6);
static unsigned long sys_getcwd(unsigned long buf, unsigned long size, unsigned long arg3,
                                unsigned long arg4, unsigned long arg5, unsigned long arg6);

/* File descriptor table */
#define MAX_FD 128
static struct {
    int used;
    const char *path;
} fd_table[MAX_FD];
static int next_fd = 10;  /* Start at 10, leave 0-9 for stdin/stdout/stderr */

/* Syscall table */
static syscall_handler_t syscall_table[64] = {
    [0] = sys_read,        /* read */
    [1] = sys_write,       /* write */
    [2] = sys_open,        /* open */
    [3] = sys_close,       /* close */
    [9] = sys_getcwd,      /* getcwd */
    [39] = sys_getpid,     /* getpid */
    [59] = sys_execve,     /* execve */
    [60] = sys_exit,      /* exit */
};

/*
 * Main syscall handler
 */
unsigned long do_syscall(unsigned long syscall_num,
                        unsigned long arg1,
                        unsigned long arg2,
                        unsigned long arg3,
                        unsigned long arg4,
                        unsigned long arg5)
{
    if (syscall_num >= 64) {
        return -1;
    }
    
    syscall_handler_t handler = syscall_table[syscall_num];
    
    if (handler) {
        return handler(arg1, arg2, arg3, arg4, arg5, 0);
    }
    
    return -1;  /* ENOSYS */
}

/*
 * sys_read - Read from file descriptor
 * For fd=0 (stdin), reads from keyboard buffer
 */
static unsigned long sys_read(unsigned long fd, unsigned long buf, unsigned long count,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg4; (void)arg5; (void)arg6;
    
    if (fd != 0) {
        return -1;  /* Only stdin supported */
    }
    
    if (!buf || count == 0) {
        return 0;
    }
    
    char* user_buf = (char*)buf;
    size_t bytes_read = 0;
    
    /* Read from keyboard buffer */
    while (bytes_read < count) {
        /* Check for available data */
        if (!keyboard_buffer_available()) {
            /* No data available, return what we have */
            break;
        }
        
        int c = keyboard_buffer_get();
        if (c < 0) {
            break;
        }
        
        user_buf[bytes_read] = (char)c;
        bytes_read++;
        
        /* Echo to console */
        console_write_char((char)c);
        
        /* Stop on newline */
        if (c == '\n' || c == '\r') {
            break;
        }
    }
    
    return bytes_read;
}

/*
 * sys_write - Write to file descriptor  
 * For fd=1 (stdout) or fd=2 (stderr), writes to console
 */
static unsigned long sys_write(unsigned long fd, unsigned long buf, unsigned long count,
                               unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg4; (void)arg5; (void)arg6;
    
    /* Only stdout and stderr supported */
    if (fd != 1 && fd != 2) {
        return -1;
    }
    
    if (!buf || count == 0) {
        return 0;
    }
    
    /* Write to console */
    console_write_string((const char*)buf, count);
    
    return count;
}

/*
 * sys_open - Open a file
 * For now, returns a virtual file descriptor
 */
static unsigned long sys_open(unsigned long pathname, unsigned long flags, unsigned long mode) {
    (void)flags;
    (void)mode;
    
    /* Find free fd */
    int fd = -1;
    for (int i = next_fd; i < MAX_FD; i++) {
        if (!fd_table[i].used) {
            fd = i;
            next_fd = i + 1;
            if (next_fd >= MAX_FD) next_fd = 10;
            fd_table[fd].used = 1;
            fd_table[fd].path = (const char*)pathname;
            break;
        }
    }
    
    if (fd < 0) {
        return -1;  /* No free fd */
    }
    
    return (unsigned long)fd;
}

/*
 * sys_close - Close a file descriptor
 */
static unsigned long sys_close(unsigned long fd) {
    if ((int)fd < 0 || fd >= MAX_FD) {
        return -1;
    }
    
    if (!fd_table[fd].used) {
        return -1;  /* Not open */
    }
    
    fd_table[fd].used = 0;
    fd_table[fd].path = NULL;
    
    return 0;
}

/*
 * sys_execve - Execute a program
 * For now, this is a stub since we pre-load binaries
 */
static unsigned long sys_execve(unsigned long filename, unsigned long argv, unsigned long envp) {
    (void)filename;
    (void)argv;
    (void)envp;
    
    /* For now, this is a no-op since binaries are pre-loaded
     * In a full implementation, this would:
     * 1. Load the binary from the given path
     * 2. Set up new address space
     * 3. Replace current process
     */
    return 0;
}

/*
 * sys_exit - Terminate current process
 * Returns to kernel (halts for now)
 */
static unsigned long sys_exit(unsigned long code, unsigned long arg2, unsigned long arg3,
                              unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    
    /* For now, just halt */
    /* In a real OS, this would return to the scheduler */
    volatile char* msg = "Process exited with code: ";
    while (*msg) {
        serial_putchar(*msg++);
    }
    serial_putchar('0' + (code % 10));
    serial_putchar('\n');
    
    while (1) {
        __asm__ volatile ("hlt");
    }
    
    return 0;
}

/*
 * sys_getpid - Get process ID
 */
static unsigned long sys_getpid(unsigned long arg1, unsigned long arg2, unsigned long arg3,
                                unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg1; (void)arg2; (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    return 1;  /* Always return 1 for init/shell */
}

/*
 * sys_getcwd - Get current working directory
 */
static unsigned long sys_getcwd(unsigned long buf, unsigned long size, unsigned long arg3,
                                unsigned long arg4, unsigned long arg5, unsigned long arg6) {
    (void)arg3; (void)arg4; (void)arg5; (void)arg6;
    
    if (!buf || size == 0) {
        return 0;
    }
    
    const char* cwd = "/";
    size_t len = 1;  /* Just "/" */
    
    if (len >= size) {
        return 0;
    }
    
    /* Copy cwd to user buffer */
    char* user_buf = (char*)buf;
    for (size_t i = 0; i < len; i++) {
        user_buf[i] = cwd[i];
    }
    user_buf[len] = '\0';
    
    return (unsigned long)buf;
}

/*
 * Initialize syscall handler
 */
void syscall_init(void) {
    set_syscall_handler_ptr((unsigned long)do_syscall);
}
