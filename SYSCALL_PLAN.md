# FeatherOS Syscall Implementation Plan

## Phase 1: Syscall Handler Infrastructure ✅
**Status: COMPLETED**

### Done
- [x] Created `include/syscall.h` with Linux-compatible syscall numbers
- [x] Created `kernel/syscall_impl.c` with `do_syscall()` handler
- [x] Fixed `interrupt.S` syscall_entry to properly pass syscall number
- [x] Added `syscall_init()` to set up handler pointer
- [x] Integrated into kernel main loop

### Files Created
- `FeatherOS/include/syscall.h` - syscall numbers
- `FeatherOS/kernel/syscall_impl.c` - C syscall handler

### Files Modified
- `FeatherOS/kernel/arch/x86_64/interrupt.S` - fixed syscall_entry
- `FeatherOS/kernel/main.c` - added syscall_init call
- `Makefile` - added syscall_impl.c to build

## Phase 2: Basic Syscalls ✅
**Status: COMPLETED (stub implementations)**

### Implemented
| Syscall | Number | Status |
|---------|--------|--------|
| read    | 0      | stub - returns 0 |
| write   | 1      | stub - returns count |
| brk     | 12     | stub - tracks brk |
| sched_yield | 24 | stub - returns 0 |
| exit    | 60     | stub - halts |

## Phase 3: User Space Entry ⏳
**Status: NEXT PHASE**

### Requirements
1. Set up user space page tables
2. Map user memory at 0x400000
3. Configure GDT with user code/data segments
4. Load user program into memory
5. Set up stack for user space
6. Return to user space via iret/sysret

### Files Needed
- `kernel/arch/x86_64/sysret.S` - return to user mode
- `kernel/exec.c` - load ELF/user program

## Phase 4: Userland Programs ⏳
**Status: DEPENDS ON PHASE 3**

### Requirements
1. Initramfs support in boot
2. Shell execution
3. Basic /dev, /proc, /sys

## Phase 5: Full Integration ⏳
**Status: DEPENDS ON PHASE 4**

### Requirements
1. Proper memory management per process
2. Process table
3. Signal handling
4. File descriptor table
5. Working file I/O syscalls

---

## Implementation Details

### Syscall Convention (Linux x86_64)
- Syscall number: RAX
- Arguments: RDI, RSI, RDX, R10, R8, R9
- Return: RAX
- Clobbered: RCX, R11

### Current Stub Behavior
- `read(0, buf, count)` - returns 0 (no data)
- `write(1, buf, count)` - returns count (fake success)
- `exit(code)` - infinite hlt loop
- Other syscalls - return -1 (ENOSYS)

### Next Steps
1. Implement actual serial/console I/O for read/write
2. Create user space entry code
3. Load initramfs with shell
4. Set up proper memory mapping for user space
