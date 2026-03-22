# Sprint 10: Process Forking

**Epic:** ./project/EPIC.md  
**Phase:** Phase 4: Extended Features  
**Status:** BACKLOG  
**Priority:** P2 (Medium)  
**Estimated Duration:** 3 days  

## Goal
Implement sys_clone/sys_fork for process creation.

## Background
After basic shell works, implement proper process forking.

## Tasks

### 10.1 Implement sys_clone (Day 1)
```c
static unsigned long sys_clone(unsigned long flags, unsigned long stack) {
    // Use copy_process from task.c
    task_t *child = copy_process(flags, stack);
    if (child) return child->pid;
    return -1;
}
```

### 10.2 Implement Copy-on-Write (Day 2)
- Mark pages as read-only in child
- Handle page fault on write
- Copy page to new physical page

### 10.3 Test parent/child (Day 3)
```bash
# Test fork()
# Parent should see child's PID
# Child should see return value 0
```

## Deliverables
- sys_clone returns child PID
- Child executes independently
- Parent waits for child

## Acceptance Criteria
```bash
# fork() test passes
# Parent waits for child
# Both execute correctly
```

## Dependencies
- Sprint 7: Basic shell working
