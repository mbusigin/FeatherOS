# Sprint 2: Fix main.c User Mode Entry

**Epic:** ./project/EPIC.md  
**Phase:** Phase 4  
**Status:** DONE  
**Priority:** P0 (Critical)  
**Estimated Duration:** 2 days  

## Goal
Modify kernel main.c to spawn user shell instead of kernel debug shell.

## Background
Current main.c runs `shell_loop()` which is a kernel debug shell. We need to spawn the user-space shell instead.

## Tasks

### 2.1 Add user mode initialization (Day 1)
- Replace `shell_loop()` call with `start_user_space()`
- Call `process_init()` to initialize process manager
- Spawn shell via `spawn_user_process("/bin/sh")`
- Start scheduler after spawning shell

### 2.2 Connect keyboard buffer to ISR (Day 1)
- Modify keyboard.c ISR to call `keyboard_buffer_put()`
- Ensure `keyboard_buffer_get()` in syscall_impl.c works
- Test keyboard interrupt fires

### 2.3 Test boot sequence (Day 2)
- Verify serial output shows "Starting user space..."
- Verify kernel doesn't panic
- Verify scheduler runs
- Add debug prints throughout

## Code Changes
```c
// In main.c, replace shell_loop() with:
void start_user_space(void) {
    console_print("Starting user space...\n");
    
    // Initialize process manager
    process_init();
    
    // Spawn shell as first user process
    int pid = spawn_user_process("/bin/sh");
    if (pid < 0) {
        console_print("ERROR: Failed to spawn shell\n");
        kernel_halt();
    }
    
    // Start scheduler (never returns)
    scheduler_start();
}
```

## Deliverables
- `main.c` modified to call `start_user_space()`
- `process_init()` called before spawning shell
- Scheduler started after shell spawns
- Debug output shows boot progress

## Acceptance Criteria
```bash
# Test: Boot test
cd project && make floppy 2>&1 | tail -5
# Should show: "Starting user space..." in serial log
```

## Dependencies
- Sprint 1: process.c must be created
- task.c: process_init() function
- schedule.c: scheduler_start() function

## Notes
- Keep kernel debug shell accessible for recovery
- Add serial debug output for troubleshooting
