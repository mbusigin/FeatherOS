# Sprint 1: Create process.c

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** DONE  
**Priority:** P0 (Critical)  
**Estimated Duration:** 3 days  

## Goal
Create the missing `kernel/process.c` file that enables user process creation.

## Background
The file `kernel/process.c` is listed in the Makefile but does not exist. This is the single most critical missing file preventing user-space shell execution.

## Tasks

### 1.1 Create process.c skeleton (Day 1)
- Create `kernel/process.c` with includes
- Define process management functions
- Include stubs for all required functions

### 1.2 Implement spawn_user_process() (Day 2)
- Allocate task struct via task.c's `allocate_task()`
- Set up user page tables using paging.c
- Load binary via exec.c's `elf_load()`
- Set up user stack via user_mode.S helpers
- Add to scheduler run queue

### 1.3 Implement load_binary() (Day 2)
- Call `elf_load()` from exec.c
- Set entry point from ELF header
- Allocate BSS region
- Zero BSS pages

### 1.4 Connect to task.c (Day 3)
- Use `copy_process()` pattern from task.c
- Set TF_USER flag
- Set up user CPU context
- Register with scheduler

## Deliverables
- `kernel/process.c` created with 300+ lines
- `spawn_user_process()` function implemented
- `load_binary()` function using exec.c
- Compiles without errors

## Acceptance Criteria
```bash
cd project && make clean && make 2>&1 | grep -c "process.c"
```

## Dependencies
- task.c (existing)
- exec.c (existing)
- paging.c (existing)
- user_mode.S (existing)

## Notes
- Use task.c as a template for structure
- Follow existing coding conventions
- Add debug serial output for tracing
