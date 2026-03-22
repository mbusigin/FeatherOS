# Sprint 6: Init Process

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** TODO  
**Priority:** P2 (Medium)  
**Estimated Duration:** 1 day  

## Goal
Make init spawn shell and handle exit gracefully.

## Background
The init process (PID 1) should be the first user process and should spawn the shell. Currently init.c is a stub.

## Tasks

### 6.1 Update init.c (Day 1)
```c
void _start(void) {
    welcome();
    print("Initializing system...\n");
    
    // For now: just run shell directly
    // In future: fork() to create shell
    extern void shell_loop(void);
    shell_loop();
    
    exit(0);
}
```

### 6.2 Handle exit gracefully (Day 1)
```c
// In syscall_impl.c sys_exit:
void do_exit(int code) {
    serial_print("Shell exited with code %d\n", code);
    // Return to kernel
    kernel_halt();
}
```

## Deliverables
- `exit` command terminates shell
- Init doesn't crash on exit
- Clean return to kernel

## Acceptance Criteria
```bash
# Test: Exit command works
# Type 'exit' in shell
# See "Goodbye!" message
# QEMU closes or returns to kernel
```

## Dependencies
- Sprint 5: Shell integration working
- init.c (existing)
- syscall_impl.c (existing)

## Notes
- Keep init simple for now
- Focus on working shell first
- Future: add proper fork/exec
