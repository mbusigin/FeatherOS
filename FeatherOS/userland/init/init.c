/*
 * FeatherOS Init Process - PID 1
 * First userspace process, spawns shell
 */

#define NULL ((void*)0)
#define stdout 1
#define stderr 2

/* Forward declarations */
int read(int fd, char *buf, int count);
int write(int fd, char *buf, int count);
int open(char *path, int flags);
void exit(int code);
int strlen(char *s);
int do_sched_yield(void);

/* Print string */
void print(char *s) {
    write(stdout, s, strlen(s));
}

void println(char *s) {
    print(s);
    print("\n");
}

void welcome(void) {
    println("");
    println("========================================");
    println("     Welcome to FeatherOS 1.0.0");
    println("========================================");
    println("");
}

void run_shell(void) {
    int pid;
    
    println("Starting shell...\n");
    
    while (1) {
        /* In a real OS, fork() would be used */
        /* For now, we just run shell directly */
        print("init: would spawn shell here\n");
        print("init: waiting forever...\n");
        
        /* Yield to scheduler */
        do_sched_yield();
    }
}

void _start(void) {
    welcome();
    run_shell();
    exit(0);
}

/* Minimal syscall stubs */
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
    return do_syscall(0, fd, (long)buf, count);
}

int write(int fd, char *buf, int count) {
    return do_syscall(1, fd, (long)buf, count);
}

int open(char *path, int flags) {
    return do_syscall(2, (long)path, flags, 0);
}

void exit(int code) {
    do_syscall(60, code, 0, 0);
    while(1) __asm__ volatile ("hlt");
}

int strlen(char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

int do_sched_yield(void) {
    return do_syscall(24, 0, 0, 0);  /* SYS_SCHED_YIELD */
}
