# FeatherOS Development Epic - v2.1

**Last Updated:** 2025-03-22  
**Goal:** Get `make run` to boot FeatherOS with a working user-space shell

---

# REALISTIC STATUS ASSESSMENT

## ⚠️ ACTUAL COMPLETION: ~25% (NOT 95% as previously claimed)

The EPIC claimed 38/40 sprints complete. This is **incorrect**. Here's the truth:

### ✅ ACTUALLY COMPLETE (8/40 sprints)

| Sprint | Description | Status | Evidence |
|--------|-------------|--------|----------|
| 3 | Boot Sector & Entry | ✅ DONE | header.S, boot_64.S, main.c work |
| 4 | Data Structures | ✅ DONE | datastructures.c has Vector, List, Hash, RB-tree |
| 5 | Physical Memory | ✅ DONE | physical.c has E820 detection, bitmap |
| 6 | Paging | ✅ DONE | paging.S, paging.c implement 4-level tables |
| 7 | Slab Allocator | ✅ DONE | slab.c functional |
| 8 | VMA | ✅ DONE | vma.c has VMA tree |
| 12 | Sync Primitives | ✅ DONE | spinlock, mutex, semaphore |
| 13 | Interrupts | ✅ DONE | IDT, ISRs, PIC/APIC |

### ⚠️ PARTIAL/STUB (5/40 sprints)

| Sprint | Description | Status | Issues |
|--------|-------------|--------|--------|
| 9 | Process Management | ⚠️ PARTIAL | task.c exists, BUT process.c MISSING (listed in Makefile but doesn't exist!) |
| 10-11 | Scheduler | ⚠️ STUBS | schedule.c has structure, context_switch.S works |
| 26-27 | Syscalls/LibC | ⚠️ STUBS | syscall_impl.c has basic read/write, but many missing |
| 28-31 | Userland | ⚠️ STUBS | shell.c and init.c exist but NOT INTEGRATED into boot |

### ❌ MISSING/INCOMPLETE (27/40 sprints)

| Sprint | Description | Status | Gap |
|--------|-------------|--------|-----|
| 14-20 | Drivers | ❌ INCOMPLETE | Code exists but not integrated into main.c |
| 21-25 | Filesystems | ❌ STUBS | vfs.c, fat32.c, ext2.c exist but untested |
| 32-40 | Build/Docs | ❌ INCOMPLETE | Makefile targets exist but `make run` doesn't work |

---

# 🚨 CRITICAL GAPS PREVENTING USER-SPACE SHELL

## GAP 1: process.c DOES NOT EXIST ⚠️

**File listed in Makefile line ~45:**
```
$(SRC_DIR)/kernel/process.c
```

**BUT the file doesn't exist!**

This is the single most critical missing file. The Makefile references it but it's not there.

## GAP 2: main.c Doesn't Spawn User Shell

Current main.c just runs a kernel debug shell:
```c
shell_loop();  // This is kernel debug shell, NOT user shell
kernel_halt();
```

**Missing:** No user process creation, no switch to user mode.

## GAP 3: No Actual Boot Integration

- `create_floppy.py` creates trivial boot sector that just prints "FeatherOS boot" and halts
- No actual kernel boot - just 16-bit real mode stub
- GRUB config exists but no way to actually run it

## GAP 4: Userland Code Not Compiled/Integrated

- Shell and init source exist in `FeatherOS/userland/`
- Makefile has `userland` and `initramfs` targets
- BUT targets are NEVER called in the build chain
- No actual initramfs archive created

## GAP 5: Syscall Gaps

In `syscall_impl.c`:
- ✅ sys_read (keyboard buffer) - works
- ✅ sys_write (console) - works  
- ✅ sys_exit - works (halts)
- ✅ sys_getpid - works
- ❌ sys_open - MISSING (shell.c calls it!)
- ❌ sys_close - MISSING
- ❌ sys_execve - MISSING
- ❌ sys_clone/fork - MISSING

## GAP 6: User Mode Entry Broken

- `user_mode.S` has the assembly functions
- BUT kernel never calls `user_space_entry` or `enter_user_mode`
- No code path from kernel shell to user shell

---

# REALISTIC IMPLEMENTATION ROADMAP

```
Current State: Kernel boots to debug shell, halts
Target State: Kernel boots, spawns user shell, interactive shell works
```

## Phase 1: Fix Critical Gaps (Week 1)

### Sprint 1: Create process.c
- [ ] Create `kernel/process.c` (file is MISSING)
- [ ] Implement `spawn_user_process()`
- [ ] Implement `load_user_binary()`
- [ ] Connect to task.c and scheduler

### Sprint 2: Fix main.c User Mode Entry
- [ ] After kernel init, spawn user shell
- [ ] Set up user page tables
- [ ] Call `enter_user_mode()`
- [ ] Verify user shell starts

### Sprint 3: Implement Missing Syscalls
- [ ] sys_open - stub file open
- [ ] sys_close - close file descriptor
- [ ] sys_execve - load and execute ELF
- [ ] sys_clone - process forking

### Sprint 4: Integrate Userland Build
- [ ] Fix `make userland` to actually compile shell.c
- [ ] Create initramfs archive
- [ ] Embed in boot image
- [ ] Fix `create_floppy.py` to boot real kernel

## Phase 2: Working Shell (Week 2)

### Sprint 5: Shell Integration
- [ ] sys_read connects to keyboard ISR
- [ ] sys_write connects to VGA/serial
- [ ] Test interactive input/output

### Sprint 6: Init Process
- [ ] init runs, spawns shell
- [ ] Ctrl+C handling
- [ ] exit returns to kernel

### Sprint 7: Integration Testing
- [ ] `make run` boots to shell
- [ ] Shell prompt appears
- [ ] Built-in commands work (echo, help, ls)
- [ ] Can type and see output

## Phase 3: Polish (Week 3)

### Sprint 8: File Operations
- [ ] VFS integration with syscalls
- [ ] Basic file reading

### Sprint 9: Testing & Documentation
- [ ] Manual testing
- [ ] Update README

---

# Phase 4: User-Space Shell (Detailed Sprint Breakdown)

The following sprints are designed to achieve the goal: `make run` should fire up FeatherOS in QEMU with a working user-space shell.

**Phase 4 contains Sprints 1-9 as detailed below:**

**Goal:** Create the missing `kernel/process.c` file that enables user process creation.

### Tasks

#### 1.1 Create process.c skeleton (Day 1)
```c
// kernel/process.c - Required file structure
#include <process.h>
#include <kernel.h>

// User process creation
int spawn_user_process(const char *path);
int load_binary(void *data, size_t size);
int setup_user_address_space(task_t *task);

// User memory management
int allocate_user_pages(task_t *task, uint64_t vaddr, size_t size);
int map_user_pages(task_t *task, uint64_t vaddr, uint64_t phys, size_t size);
```

#### 1.2 Implement spawn_user_process() (Day 2)
- Allocate task struct via task.c's `allocate_task()`
- Set up user page tables using paging.c
- Load binary via exec.c's `elf_load()`
- Set up user stack via user_mode.S helpers
- Add to scheduler run queue

#### 1.3 Implement load_binary() (Day 2)
- Call `elf_load()` from exec.c
- Set entry point from ELF header
- Allocate BSS region
- Zero BSS pages

#### 1.4 Connect to task.c (Day 3)
- Use `copy_process()` pattern from task.c
- Set TF_USER flag
- Set up user CPU context
- Register with scheduler

### Acceptance Criteria
```bash
# Test: process.c compiles
cd project && make clean && make 2>&1 | grep -c "process.c"  # Should show compilation
# No "file not found" errors
```

### Deliverables
- `kernel/process.c` created with 300+ lines
- `spawn_user_process()` function implemented
- `load_binary()` function using exec.c

---

## Sprint 2: Fix main.c User Mode Entry (2 days)

**Goal:** Modify kernel main.c to spawn user shell instead of kernel debug shell.

### Tasks

#### 2.1 Add user mode initialization (Day 1)
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

#### 2.2 Connect keyboard buffer to ISR (Day 1)
- Modify keyboard.c ISR to call `keyboard_buffer_put()`
- Ensure `keyboard_buffer_get()` in syscall_impl.c works

#### 2.3 Test boot sequence (Day 2)
- Verify serial output shows "Starting user space..."
- Verify kernel doesn't panic
- Verify scheduler runs

### Acceptance Criteria
```bash
# Test: Boot test
cd project && make floppy 2>&1 | tail -5
# Should show: "Starting user space..." in serial log
```

### Deliverables
- `main.c` modified to call `start_user_space()`
- `process_init()` called before spawning shell
- Scheduler started after shell spawns

---

## Sprint 3: Implement Missing Syscalls (3 days)

**Goal:** Implement sys_open, sys_close, sys_execve so shell can work.

### Tasks

#### 3.1 Implement sys_open (Day 1)
```c
static unsigned long sys_open(unsigned long pathname, unsigned long flags,
                              unsigned long mode) {
    (void)flags; (void)mode;
    // Stub: return virtual FD number
    // Later: use VFS to open real file
    static int next_fd = 10;
    return next_fd++;
}
```

#### 3.2 Implement sys_close (Day 1)
```c
static unsigned long sys_close(unsigned long fd) {
    // Stub: just return success
    if (fd >= 0 && fd < 128) return 0;
    return -1;
}
```

#### 3.3 Implement sys_execve (Day 2)
```c
static unsigned long sys_execve(unsigned long filename, 
                                unsigned long argv, unsigned long envp) {
    (void)filename; (void)argv; (void)envp;
    // For shell: we pre-load it, so this is a no-op
    // Later: load new binary
    return 0;
}
```

#### 3.4 Add syscall to table (Day 3)
```c
// In syscall_impl.c syscall_table:
[2] = sys_open,    // SYS_open
[3] = sys_close,   // SYS_close  
[59] = sys_execve, // SYS_execve
```

### Acceptance Criteria
```bash
# Test: Shell compiles without undefined references
cd project && gcc -c FeatherOS/userland/shell/shell.c 2>&1 | grep -c error
# Should be 0
```

### Deliverables
- `sys_open` returns valid FD
- `sys_close` accepts any FD
- `sys_execve` stubs successfully
- Shell's open/close calls work

---

## Sprint 4: Integrate Userland Build (2 days)

**Goal:** Fix `make userland` and `make run` to actually work.

### Tasks

#### 4.1 Fix userland compilation (Day 1)
```makefile
# In Makefile, fix userland target:
userland:
	mkdir -p $(BUILD_DIR)/initramfs/bin $(BUILD_DIR)/initramfs/sbin
	$(CC) $(CFLAGS) -nostdlib -fno-pie -e _start \
		-o $(BUILD_DIR)/initramfs/shell \
		$(SRC_DIR)/userland/shell/shell.c
```

#### 4.2 Create initramfs (Day 1)
```bash
# Create directory structure
mkdir -p build/initramfs/bin build/initramfs/sbin
# Copy binaries
cp build/initramfs/shell build/initramfs/bin/sh
cp build/initramfs/shell build/initramfs/sbin/init
# Create cpio archive
cd build/initramfs && find . -type f | cpio -o -H newc > ../initramfs.cpio
```

#### 4.3 Fix create_floppy.py (Day 2)
- Copy kernel.bin to floppy at correct offset (0x2000)
- Set up proper MBR boot signature
- Ensure BIOS loads kernel at 0x100000

#### 4.4 Update Makefile run target (Day 2)
```makefile
run: userland floppy
	qemu-system-x86_64 -fda featheros.img -m 256M \
		-serial file:serial.log -no-reboot
```

### Acceptance Criteria
```bash
# Test: Full build
cd project && make clean && make userland 2>&1 | tail -10
# Should show: "Userland build complete"
ls -la build/initramfs/shell  # Binary exists
```

### Deliverables
- `make userland` produces shell binary
- `make run` launches QEMU
- Serial log shows boot messages

---

## Sprint 5: Shell Integration (2 days)

**Goal:** Ensure shell input/output works correctly via syscalls.

### Tasks

#### 5.1 Debug keyboard buffer (Day 1)
- Add debug prints to `keyboard_buffer_put()`
- Verify ISR fires on keypress
- Check buffer doesn't overflow

#### 5.2 Debug console output (Day 2)
- Test `sys_write` outputs to VGA
- Test `sys_write` outputs to serial
- Verify `console_write_char()` works

#### 5.3 Test interactive loop (Day 2)
```bash
# Manual test in QEMU
qemu-system-x86_64 -fda featheros.img -serial mon:stdio
# Type 'echo hello' - should see output
```

### Acceptance Criteria
```bash
# Test: Interactive test (requires manual verification)
# echo command produces output
# help command shows help text
# Cursor moves as you type
```

### Deliverables
- Keyboard input reaches shell
- Shell output appears on screen
- No infinite loops or crashes

---

## Sprint 6: Init Process (1 day)

**Goal:** Make init spawn shell and handle exit.

### Tasks

#### 6.1 Update init.c (Day 1)
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

#### 6.2 Handle exit gracefully (Day 1)
```c
// In syscall_impl.c sys_exit:
void do_exit(int code) {
    serial_print("Shell exited with code %d\n", code);
    // Return to kernel
    kernel_halt();
}
```

### Acceptance Criteria
```bash
# Test: Exit command works
# Type 'exit' in shell
# See "Goodbye!" message
# QEMU closes or returns to kernel
```

### Deliverables
- `exit` command terminates shell
- Init doesn't crash on exit
- Clean return to kernel

---

## Sprint 7: Integration Testing (2 days)

**Goal:** Verify `make run` produces working system.

### Tasks

#### 7.1 Build test (Day 1)
```bash
make clean && make
make userland
make run
```

#### 7.2 Manual verification (Day 1-2)
- [ ] Shell prompt appears
- [ ] `echo hello` prints "hello"
- [ ] `help` shows commands
- [ ] `pwd` shows "/"
- [ ] `ls` shows "."
- [ ] `exit` terminates

#### 7.3 Create test script (Day 2)
```bash
#!/bin/bash
# test_shell.sh
make clean && make && make userland
timeout 5 qemu-system-x86_64 -fda featheros.img \
    -serial file:test_serial.log -nographic || true
grep "echo hello" test_serial.log && echo "PASS" || echo "FAIL"
```

### Acceptance Criteria
```bash
# Test: make run produces working shell
make run 2>&1 | head -20
# QEMU launches with serial output
# Serial log contains shell output
```

### Deliverables
- Working `make run` command
- Test script for regression
- Documentation of expected output

---

# SUCCESS CRITERIA

## Sprint 1-4 (Week 1): Target
- [ ] `make` compiles without errors
- [ ] process.c exists and compiles
- [ ] Kernel boots past init
- [ ] User page tables set up

## Sprint 5-7 (Week 2): Target
- [ ] `make run` launches QEMU
- [ ] Shell prompt appears: `featheros:/$ `
- [ ] `echo hello` works
- [ ] `help` command works
- [ ] `exit` terminates shell

## Sprint 8-9 (Week 3): Target
- [ ] `cat /etc motd` works (if file exists)
- [ ] Documentation updated
- [ ] README shows how to use

---

# FILE STATUS TABLE

| File | Status | Notes |
|------|--------|-------|
| `kernel/main.c` | ⚠️ PARTIAL | Runs kernel shell only |
| `kernel/process.c` | ❌ MISSING | Listed in Makefile, doesn't exist! |
| `kernel/syscall_impl.c` | ⚠️ PARTIAL | Basic syscalls, missing open/exec |
| `kernel/exec.c` | ⚠️ PARTIAL | ELF loader with stubs |
| `kernel/sched/task.c` | ✅ DONE | PID allocator, kernel threads |
| `kernel/sched/schedule.c` | ⚠️ STUBS | CFS structure, not fully integrated |
| `kernel/arch/x86_64/user_mode.S` | ✅ DONE | User entry assembly ready |
| `kernel/arch/x86_64/syscall.S` | ✅ DONE | Syscall entry point |
| `userland/shell/shell.c` | ✅ DONE | Simple shell with built-ins |
| `userland/init/init.c` | ⚠️ STUBS | Init process stub |
| `create_floppy.py` | ❌ BROKEN | Only prints "boot", doesn't boot kernel |

---

# UPDATED SPRINT TIMELINE

| Phase | Sprint | Status | Focus | Owner |
|-------|--------|--------|-------|-------|
| 1 | 1 | 🔴 TODO | Create process.c | - |
| 1 | 2 | 🔴 TODO | Fix main.c user mode entry | - |
| 1 | 3 | 🔴 TODO | Implement missing syscalls | - |
| 1 | 4 | 🔴 TODO | Integrate userland build | - |
| 2 | 5 | 🔴 TODO | Shell integration | - |
| 2 | 6 | 🔴 TODO | Init process | - |
| 2 | 7 | 🔴 TODO | Integration testing | - |
| 3 | 8 | 🔴 TODO | File operations | - |
| 3 | 9 | 🔴 TODO | Testing & docs | - |

---

# RISK REGISTER

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| process.c creation takes too long | High | High | Use task.c as template |
| User page tables broken | High | High | Debug serial output |
| Keyboard ISR not firing | Medium | Medium | Verify PIC config |
| Shell syscall stubs fail | Medium | Medium | Add debug prints |

---

# NOTES

- Previous EPIC claimed 95% complete - this was **wrong**
- Real progress is ~25% - kernel infrastructure exists but not integrated
- Main blocker: process.c missing, no user mode entry, no boot integration
- Fix order: process.c → main.c → syscalls → userland integration → test

*This epic is the living document for FeatherOS. Current focus: Sprints 1-7 to achieve basic user-space shell.*
