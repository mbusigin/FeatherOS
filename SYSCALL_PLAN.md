# FeatherOS Syscall Implementation Plan

## Overview
Implement real syscall support to enable userland program execution in FeatherOS.

## Phases

### Phase 1: Syscall Handler (kernel/arch/x86_64/syscall.S)
- [ ] Define syscall numbers in include/syscall.h
- [ ] Implement syscall entry point (syscall instruction)
- [ ] Save/restore registers
- [ ] Call C handler
- [ ] Return to user space

### Phase 2: Basic Syscalls (kernel/syscall.c)
- [ ] sys_read - read from fd
- [ ] sys_write - write to fd  
- [ ] sys_exit - terminate process
- [ ] sys_brk - change data segment

### Phase 3: Process Syscalls
- [ ] sys_fork - create process
- [ ] sys_execve - execute program
- [ ] sys_wait4 - wait for process

### Phase 4: User Space Entry
- [ ] Map user space memory (0x400000)
- [ ] Set up user stack
- [ ] Load initramfs into memory
- [ ] Jump to user mode (iretq)

### Phase 5: Integration
- [ ] Modify main.c to spawn init
- [ ] Bundle initramfs into boot image
- [ ] Test with shell

## Syscall Numbers (Linux-compatible)
| Number | Name | Description |
|--------|------|-------------|
| 0 | read | Read from fd |
| 1 | write | Write to fd |
| 2 | open | Open file |
| 3 | close | Close fd |
| 9 | mmap | Memory map |
| 10 | mprotect | Set protection |
| 11 | munmap | Unmap memory |
| 12 | brk | Change data segment |
| 21 | access | Check file access |
| 59 | execve | Execute program |
| 60 | exit | Terminate |
| 231 | exit_group | Exit all threads |
| 60 | exit | Exit process |

## Implementation Files
- `FeatherOS/include/syscall.h` - Syscall numbers and definitions
- `FeatherOS/kernel/arch/x86_64/syscall.S` - Assembly entry point
- `FeatherOS/kernel/syscall.c` - C syscall handlers
- `FeatherOS/kernel/exec.c` - Program loading

## Test
```bash
make clean && make all && make run
# Should see shell prompt in QEMU
```
