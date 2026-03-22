# FeatherOS Development Epic - v2.0

**Last Updated:** 2025-03-22  
**Goal:** Get `make run` to boot FeatherOS with a working user-space shell

---

# CURRENT STATUS ASSESSMENT

## ✅ COMPLETED (38/40 sprints)

| Sprint | Description | Status |
|--------|-------------|--------|
| 1-3 | Foundation & Bootstrapping | ✅ DONE |
| 4-6 | Kernel Core & Data Structures | ✅ DONE |
| 7-8 | Memory Management (Slab, VMA) | ✅ DONE |
| 9-12 | Process, Context, Scheduler, Sync | ✅ DONE |
| 13-20 | Device Drivers & Interrupt | ✅ DONE |
| 21-25 | Filesystem (VFS, FAT, ext2) | ✅ DONE |
| 26 | LibC (string, stdio) | ✅ DONE |
| 27 | System Call Interface | ✅ DONE |
| 28-31 | Userland (stub implementations) | ⚠️ PARTIAL |
| 32-40 | Build, Testing, Documentation | ⚠️ PARTIAL |

## 🚨 CRITICAL GAPS TO ACHIEVE USER-SPACE SHELL

### 1. Userland Execution Infrastructure (MISSING)
- [ ] `exec.c` - Load ELF binaries into memory
- [ ] `process.c` - Full process management
- [ ] User space page tables and memory mapping
- [ ] User space entry via iret/sysret

### 2. Working Syscalls (STUBS ONLY)
- [ ] `sys_read` - Keyboard input → returns 0 (broken)
- [ ] `sys_write` - Console output → returns count (broken)
- [ ] `sys_execve` - Not implemented
- [ ] `sys_clone/fork` - Not implemented

### 3. Initramfs & Boot Integration (MISSING)
- [ ] Initramfs directory structure
- [ ] Bundle shell/init into boot image
- [ ] GRUB config for initramfs
- [ ] Kernel command line with init=

### 4. Serial I/O for User Programs (MISSING)
- [ ] Connect stdin to keyboard buffer
- [ ] Connect stdout to VGA/serial

---

# PHASE 7-B: USERLAND EXECUTION (NEW)

## Sprint 60: User Space Memory Setup

### Tasks
- [ ] Design user virtual address space layout (0x400000 for code)
- [ ] Implement user page table setup
- [ ] Map user memory with appropriate permissions (US bit)
- [ ] Set up user stack (0x7fff_ffff_ffff_0000)
- [ ] Configure GDT with user code/data segments (0x23/0x1B)

### Acceptance Criteria
- [ ] Can switch to user mode via iret
- [ ] User code executes with CPL=3
- [ ] User memory is isolated from kernel

### Deliverable: `kernel/arch/x86_64/user_mode.S`

---

## Sprint 61: ELF Binary Loader

### Tasks
- [ ] Parse ELF64 header (e_ident, e_type, e_machine)
- [ ] Read program headers (PT_LOAD)
- [ ] Copy segments to user memory
- [ ] Set up brk, heap, mmap regions
- [ ] Validate ELF (x86_64 architecture check)

### Acceptance Criteria
- [ ] Can load simple static ELF binary
- [ ] Entry point jumped to correctly
- [ ] Segments mapped at correct addresses

### Deliverable: `kernel/exec.c`

---

## Sprint 62: Working Syscalls

### Tasks
- [ ] Implement `sys_read` - Read from stdin buffer
- [ ] Implement `sys_write` - Write to VGA/serial console
- [ ] Implement `sys_open` - Open file (stub)
- [ ] Implement `sys_exit` - Return to kernel/shell
- [ ] Connect keyboard ISR to stdin buffer

### Acceptance Criteria
- [ ] Shell can print output to screen
- [ ] Shell can read keyboard input
- [ ] Commands execute and show results

### Deliverable: `kernel/syscall_impl.c` (updated)

---

## Sprint 63: Initramfs & Boot Integration

### Tasks
- [ ] Create initramfs directory structure
- [ ] Compile shell and init with userland toolchain
- [ ] Create cpio archive of initramfs
- [ ] Embed initramfs in boot image
- [ ] Add init= kernel parameter
- [ ] Load and extract initramfs at boot

### Acceptance Criteria
- [ ] Shell binary loaded from initramfs
- [ ] Kernel spawns shell as init process
- [ ] `/bin/sh` works interactively

### Deliverable: `build/initramfs/`, `Makefile` targets

---

## Sprint 64: Full Userland Integration

### Tasks
- [ ] Implement `sys_clone`/`sys_fork` for process creation
- [ ] Set up per-process file descriptor table
- [ ] Implement `sys_execve` to run programs
- [ ] Add signal handling (SIGINT for Ctrl+C)
- [ ] Virtual terminal switching (Alt+F1-F6)

### Acceptance Criteria
- [ ] Can run multiple shell instances
- [ ] Ctrl+C terminates foreground process
- [ ] Basic utilities (ls, cat) work

### Test Command
```bash
cd /project && make run
# Expected: FeatherOS shell prompt "$ "
```

---

# PHASE 10: STRETCH GOALS (OPTIONAL)

## Sprint 65: Working File I/O

### Tasks
- [ ] Implement VFS-based file operations
- [ ] Connect FAT32 driver to sys_open/read/write
- [ ] Implement file descriptor table per process

## Sprint 66: Basic Utilities

### Tasks
- [ ] Implement ls, pwd, cd commands
- [ ] Implement cat, echo commands
- [ ] Implement mkdir, rm commands

## Sprint 67: Multitasking

### Tasks
- [ ] Implement process forking
- [ ] Add background job support
- [ ] Implement wait() for parent/child

---

# SPRINT TIMELINE (UPDATED)

| Phase | Sprint | Status | Focus |
|-------|--------|--------|-------|
| 1 | 1-3 | ✅ DONE | Foundation & Bootstrapping |
| 2 | 4-6 | ✅ DONE | Kernel Core |
| 3 | 7-8 | ✅ DONE | Memory Management |
| 4 | 9-12 | ✅ DONE | Process & Scheduling |
| 5 | 13-20 | ✅ DONE | Device Drivers |
| 6 | 21-25 | ✅ DONE | Filesystem |
| 7 | 26-31 | ⚠️ PARTIAL | Userland & System Calls |
| **NEW** | **60-64** | 🔲 TODO | **Userland Execution** |
| 8 | 32-36 | ⚠️ PARTIAL | Build & Testing |
| 9 | 37-40 | ⚠️ PARTIAL | Polish & Documentation |

---

# IMPLEMENTATION ROADMAP

```
Phase 7-B: Userland Execution
├── Sprint 60: User Space Memory Setup (2-3 days)
│   └── kernel/arch/x86_64/user_mode.S
├── Sprint 61: ELF Binary Loader (2-3 days)
│   └── kernel/exec.c
├── Sprint 62: Working Syscalls (2-3 days)
│   └── kernel/syscall_impl.c (update)
├── Sprint 63: Initramfs & Boot (2-3 days)
│   └── build/initramfs/, Makefile targets
└── Sprint 64: Full Integration (2-3 days)
    └── make run → shell works!
```

---

# SUCCESS CRITERIA

## Minimum Viable Product (Sprint 60-64)
- [ ] `make run` boots FeatherOS
- [ ] Shell prompt appears
- [ ] Can type commands
- [ ] Commands execute (echo, pwd, help)
- [ ] `exit` or `quit` terminates shell

## Extended Goals (Sprint 65-67)
- [ ] File operations (cat, ls)
- [ ] Process forking
- [ ] Multiple terminals

---

# FILE STRUCTURE (UPDATED)

```
/project/
├── FeatherOS/
│   ├── boot/
│   ├── kernel/
│   │   ├── arch/x86_64/
│   │   │   ├── user_mode.S        # NEW: User space entry
│   │   │   └── syscall.S
│   │   ├── exec.c                  # NEW: ELF loader
│   │   ├── syscall_impl.c          # UPDATED: Working syscalls
│   │   └── process.c               # NEW: Full process mgmt
│   ├── userland/
│   │   ├── shell/shell.c           # EXISTS
│   │   └── init/init.c             # EXISTS
│   └── ...
├── build/
│   ├── initramfs/                  # NEW: Cpio archive
│   │   ├── bin/sh
│   │   └── sbin/init
│   └── kernel.bin
├── Makefile                        # UPDATED: userland targets
└── EPIC.md
```

---

# RISK REGISTER (UPDATED)

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| ELF parsing bugs | High | Medium | Start with flat binary, add ELF later |
| Syscall arg passing | High | High | Verify register convention carefully |
| Initramfs not loading | Medium | High | Debug serial output, test step-by-step |
| Shell input buffering | Medium | Low | Add ring buffer for keyboard |

---

*This epic is the living document for FeatherOS. Current focus: Sprint 60-64 to achieve working user-space shell.*
