# Sprint 11: Signal Handling

**Epic:** ./project/EPIC.md  
**Phase:** Phase 4: Extended Features  
**Status:** BACKLOG  
**Priority:** P2 (Medium)  
**Estimated Duration:** 2 days  

## Goal
Implement signal handling for Ctrl+C and basic signals.

## Background
Shell needs to handle Ctrl+C to interrupt running processes.

## Tasks

### 11.1 Implement SIGINT (Day 1)
```c
// Handle Ctrl+C (SIGINT)
void handle_keyboard_interrupt(void) {
    // Send SIGINT to foreground process
    send_signal(foreground_pid, SIGINT);
}

// Signal delivery
void do_signal(int signum) {
    // Call signal handler or use default
    if (current_task->sig_handler[signum]) {
        // Jump to handler
    } else {
        // Default action
    }
}
```

### 11.2 Signal handler registration (Day 1)
```c
static unsigned long sys_sigaction(unsigned long signum,
                                   unsigned long act, unsigned long oldact) {
    // Register signal handler
    if (act) current_task->sig_handler[signum] = act;
    if (oldact) *oldact = current_task->sig_handler[signum];
    return 0;
}
```

### 11.3 Test Ctrl+C (Day 2)
```bash
# Run a long command
# Press Ctrl+C
# Command should terminate
```

## Deliverables
- SIGINT delivered on Ctrl+C
- Processes can register handlers
- Default actions work

## Acceptance Criteria
```bash
# Ctrl+C terminates foreground process
# Signal handlers can be registered
```

## Dependencies
- Sprint 10: Process forking
