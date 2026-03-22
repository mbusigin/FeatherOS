# Sprint 3: Implement Missing Syscalls

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** TODO  
**Priority:** P0 (Critical)  
**Estimated Duration:** 3 days  

## Goal
Implement sys_open, sys_close, sys_execve so shell can work.

## Background
The shell.c calls sys_open and sys_close, but these are not implemented in syscall_impl.c. The shell will fail to compile without these.

## Tasks

### 3.1 Implement sys_open (Day 1)
```c
static unsigned long sys_open(unsigned long pathname, unsigned long flags,
                              unsigned long mode) {
    (void)pathname; (void)flags; (void)mode;
    // Stub: return virtual FD number
    // Later: use VFS to open real file
    static int next_fd = 10;
    return next_fd++;
}
```

### 3.2 Implement sys_close (Day 1)
```c
static unsigned long sys_close(unsigned long fd) {
    // Stub: just return success
    if (fd >= 0 && fd < 128) return 0;
    return -1;
}
```

### 3.3 Implement sys_execve (Day 2)
```c
static unsigned long sys_execve(unsigned long filename, 
                                unsigned long argv, unsigned long envp) {
    (void)filename; (void)argv; (void)envp;
    // For shell: we pre-load it, so this is a no-op
    // Later: load new binary
    return 0;
}
```

### 3.4 Add syscalls to table (Day 3)
- Add to syscall_table in syscall_impl.c
- sys_open → index 2
- sys_close → index 3
- sys_execve → index 59

## Deliverables
- `sys_open` returns valid FD
- `sys_close` accepts any FD
- `sys_execve` stubs successfully
- Shell's open/close calls work

## Acceptance Criteria
```bash
# Test: Shell compiles without undefined references
cd project && gcc -c FeatherOS/userland/shell/shell.c 2>&1 | grep -c error
# Should be 0
```

## Dependencies
- shell.c (existing)
- syscall_impl.c (existing)

## Notes
- These are stubs - full implementation comes later
- Add debug prints to track syscall usage
