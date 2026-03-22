/*
 * FeatherOS Simple Shell - Minimal implementation
 * Built-in commands only, no external libc needed
 */

#define NULL ((void*)0)
#define EOF -1
#define stdin 0
#define stdout 1
#define stderr 2

/* Minimal syscall numbers for FeatherOS */
#define SYS_READ 0
#define SYS_WRITE 1
#define SYS_OPEN 2
#define SYS_CLOSE 3
#define SYS_EXIT 60

/* File modes */
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64

/* Forward declarations */
int syscall(int num, long a1, long a2, long a3);
int read(int fd, char *buf, int count);
int write(int fd, char *buf, int count);
int open(char *path, int flags);
int close(int fd);
void exit(int code);
int strlen(char *s);
char *strcpy(char *dest, char *src);
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, int n);

/* I/O buffer */
char line[256];
char *args[32];

/* Built-in commands */
int cmd_echo(char **argv);
int cmd_pwd(void);
int cmd_cd(char **argv);
int cmd_exit(char **argv);
int cmd_help(void);
int cmd_ls(char **argv);
int cmd_cat(char **argv);

/* Parse command line */
int parse_line(char *line, char **args) {
    int argc = 0;
    char *token = line;
    int in_token = 0;
    
    for (int i = 0; line[i]; i++) {
        if (line[i] == ' ' || line[i] == '\t' || line[i] == '\n') {
            if (in_token) {
                line[i] = '\0';
                args[argc++] = token;
                in_token = 0;
            }
        } else {
            if (!in_token) {
                token = &line[i];
                in_token = 1;
            }
        }
    }
    args[argc] = NULL;
    return argc;
}

/* Print string */
void print(char *s) {
    write(stdout, s, strlen(s));
}

/* Print with newline */
void println(char *s) {
    print(s);
    print("\n");
}

/* Read line from stdin */
int read_line(void) {
    int i = 0;
    char c;
    
    while (i < 255) {
        if (read(stdin, &c, 1) <= 0) {
            return -1;
        }
        if (c == '\n') {
            line[i++] = '\n';
            break;
        }
        line[i++] = c;
    }
    line[i] = '\0';
    return i;
}

/* Execute command */
int execute(char **args) {
    if (args[0] == NULL) return 1;
    
    if (strcmp(args[0], "echo") == 0) return cmd_echo(args);
    if (strcmp(args[0], "pwd") == 0) return cmd_pwd();
    if (strcmp(args[0], "cd") == 0) return cmd_cd(args);
    if (strcmp(args[0], "exit") == 0) return cmd_exit(args);
    if (strcmp(args[0], "quit") == 0) return cmd_exit(args);
    if (strcmp(args[0], "help") == 0) return cmd_help();
    if (strcmp(args[0], "ls") == 0) return cmd_ls(args);
    if (strcmp(args[0], "cat") == 0) return cmd_cat(args);
    
    print("shell: ");
    print(args[0]);
    println(": command not found");
    return 1;
}

/* Built-in: echo */
int cmd_echo(char **argv) {
    for (int i = 1; argv[i]; i++) {
        print(argv[i]);
        if (argv[i+1]) print(" ");
    }
    print("\n");
    return 1;
}

/* Built-in: pwd */
int cmd_pwd(void) {
    println("/");
    return 1;
}

/* Built-in: cd */
int cmd_cd(char **argv) {
    if (argv[1]) {
        /* In a real OS, this would call chdir syscall */
        /* For now, just acknowledge */
    }
    return 1;
}

/* Built-in: exit */
int cmd_exit(char **argv) {
    println("Goodbye!");
    return 0;
}

/* Built-in: help */
int cmd_help(void) {
    println("FeatherOS Shell - Available commands:");
    println("  echo <text>  - Print text");
    println("  pwd          - Print working directory");
    println("  cd <dir>     - Change directory");
    println("  ls           - List files");
    println("  cat <file>   - Show file contents");
    println("  help         - Show this help");
    println("  exit         - Exit shell");
    return 1;
}

/* Built-in: ls */
int cmd_ls(char **argv) {
    (void)argv;
    /* Simple listing */
    println(".");
    println("..");
    return 1;
}

/* Built-in: cat */
int cmd_cat(char **argv) {
    if (!argv[1]) {
        println("cat: missing file operand");
        return 1;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        print("cat: ");
        print(argv[1]);
        println(": No such file");
        return 1;
    }
    
    char buf[256];
    int n;
    while ((n = read(fd, buf, 256)) > 0) {
        write(stdout, buf, n);
    }
    close(fd);
    return 1;
}

/* Main shell loop */
void shell_loop(void) {
    println("=== FeatherOS Shell ===");
    println("Type 'help' for available commands.");
    println("");
    
    while (1) {
        print("featheros:/$ ");
        
        if (read_line() <= 0) {
            break;
        }
        
        /* Parse */
        int argc = parse_line(line, args);
        
        /* Execute */
        if (!execute(args)) {
            break;
        }
    }
}

/* Entry point */
void _start(void) {
    shell_loop();
    exit(0);
}

/* Syscall stub - uses syscall instruction */
static inline long do_syscall(long num, long a1, long a2, long a3) {
    long ret;
    __asm__ volatile (
        "syscall"
        : "=a"(ret)
        : "a"(num), "D"(a1), "S"(a2), "d"(a3)
    );
    return ret;
}

int read(int fd, char *buf, int count) {
    return do_syscall(SYS_READ, fd, (long)buf, count);
}

int write(int fd, char *buf, int count) {
    return do_syscall(SYS_WRITE, fd, (long)buf, count);
}

int open(char *path, int flags) {
    return do_syscall(SYS_OPEN, (long)path, flags, 0);
}

int close(int fd) {
    return do_syscall(SYS_CLOSE, fd, 0, 0);
}

void exit(int code) {
    do_syscall(SYS_EXIT, code, 0, 0);
    /* If syscall returns, halt */
    while(1) __asm__ volatile ("hlt");
}

int strlen(char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

char *strcpy(char *dest, char *src) {
    int i = 0;
    while ((dest[i] = src[i])) i++;
    return dest;
}

int strcmp(char *s1, char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

int strncmp(char *s1, char *s2, int n) {
    while (n-- > 0 && *s1 && *s2) {
        if (*s1 != *s2) return *s1 - *s2;
        s1++;
        s2++;
    }
    return 0;
}
