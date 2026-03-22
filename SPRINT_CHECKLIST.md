# FeatherOS Development Sprint Checklist

## Quick Reference

| Phase | Sprints | Duration | Status |
|-------|---------|----------|--------|
| 1. Foundation & Bootstrapping | 1-3 | 3 sprints | ⬜ |
| 2. Kernel Core | 4-6 | 3 sprints | ⬜ |
| 3. Memory Management | 7-8 | 2 sprints | ⬜ |
| 4. Process & Scheduling | 9-12 | 4 sprints | ⬜ |
| 5. Device Drivers | 13-20 | 8 sprints | ⬜ |
| 6. Filesystem | 21-25 | 5 sprints | ⬜ |
| 7. Userland & System Calls | 26-31 | 6 sprints | ⬜ |
| 8. Build & Testing | 32-36 | 5 sprints | ⬜ |
| 9. Polish & Documentation | 37-40 | 4 sprints | ⬜ |

**Total: 40 sprints (~80 weeks)**

---

## Sprint-by-Sprint Checklist

### Phase 1: Foundation & Bootstrapping

- [ ] **Sprint 1:** Project Setup & Build System
  - [ ] Git repository initialized
  - [ ] Cross-compiler configured
  - [ ] Makefile system operational
  - [ ] CI/CD pipeline configured

- [ ] **Sprint 2:** Bootloader & Bare Metal Entry
  - [ ] GRUB2 multiboot2 header
  - [ ] 32-bit to 64-bit transition
  - [ ] GDT configured
  - [ ] Initial page tables

- [ ] **Sprint 3:** Console & Basic I/O
  - [ ] VGA text mode
  - [ ] Serial port (COM1)
  - [ ] Keyboard driver
  - [ ] printk() function

### Phase 2: Kernel Core

- [ ] **Sprint 4:** Kernel Data Structures
  - [ ] Vector, list, hash table, RB-tree
  - [ ] kmalloc/kfree
  - [ ] Unit tests passing

- [ ] **Sprint 5:** Physical Memory Management
  - [ ] E820 detection
  - [ ] Bitmap allocator
  - [ ] Memory statistics

- [ ] **Sprint 6:** Virtual Memory (Paging)
  - [ ] 4-level page tables
  - [ ] Page fault handler
  - [ ] vmalloc/vfree
  - [ ] Per-process page tables

### Phase 3: Memory Management

- [ ] **Sprint 7:** Slab Allocator
  - [ ] Object caches
  - [ ] Per-CPU caches
  - [ ] Cache reaping

- [ ] **Sprint 8:** Virtual Memory Areas (VMAs)
  - [ ] VMA structures
  - [ ] mmap/munmap syscalls
  - [ ] /proc/PID/maps

### Phase 4: Process & Scheduling

- [ ] **Sprint 9:** Process Structure & Creation
  - [ ] task_struct defined
  - [ ] kernel_thread()
  - [ ] copy_process() / fork()
  - [ ] PID allocator

- [ ] **Sprint 10:** Context Switching
  - [ ] switch_to() assembly
  - [ ] FPU state saving
  - [ ] Stack switching

- [ ] **Sprint 11:** Scheduler
  - [ ] CFS algorithm
  - [ ] Per-CPU run queues
  - [ ] Priority support

- [ ] **Sprint 12:** Synchronization Primitives
  - [ ] Atomic operations
  - [ ] Spinlocks
  - [ ] Mutexes, semaphores
  - [ ] Memory barriers

### Phase 5: Device Drivers

- [ ] **Sprint 13:** Interrupt Handling
  - [ ] IDT setup
  - [ ] ISR handlers
  - [ ] PIC/APIC
  - [ ] Bottom halves

- [ ] **Sprint 14:** Timer & Clock
  - [ ] PIT driver
  - [ ] TSC timekeeping
  - [ ] HPET driver
  - [ ] Timer wheel

- [ ] **Sprint 15:** PS/2 Keyboard & Mouse
  - [ ] Keyboard driver
  - [ ] Mouse driver
  - [ ] Terminal input

- [ ] **Sprint 16:** VGA/Framebuffer
  - [ ] Framebuffer driver
  - [ ] 2D blitting
  - [ ] Font rendering
  - [ ] Terminal emulator

- [ ] **Sprint 17:** Storage (ATA/PATA)
  - [ ] ATA PIO mode
  - [ ] DMA support
  - [ ] Block device layer
  - [ ] Partition detection

- [ ] **Sprint 18:** AHCI (SATA)
  - [ ] AHCI detection
  - [ ] FIS handling
  - [ ] NCQ support
  - [ ] Hotplug

- [ ] **Sprint 19:** Network Stack (Basic)
  - [ ] Ethernet RX/TX
  - [ ] ARP
  - [ ] Loopback device

- [ ] **Sprint 20:** TCP/IP Stack
  - [ ] IPv4
  - [ ] TCP state machine
  - [ ] UDP
  - [ ] Socket API

### Phase 6: Filesystem

- [ ] **Sprint 21:** Virtual File System (VFS)
  - [ ] vnode/superblock
  - [ ] File descriptor table
  - [ ] Path resolution
  - [ ] Dentry cache

- [ ] **Sprint 22:** RAM Disk & DevFS
  - [ ] RAM disk driver
  - [ ] devtmpfs
  - [ ] /dev/null, /dev/zero
  - [ ] tmpfs, fifofs

- [ ] **Sprint 23:** FAT32 Filesystem
  - [ ] FAT32 structure
  - [ ] Directory entries
  - [ ] Long filename support
  - [ ] fsck

- [ ] **Sprint 24:** ext2/ext4 Filesystem
  - [ ] ext2 superblock
  - [ ] Inode handling
  - [ ] ext4 extents
  - [ ] Journaling

- [ ] **Sprint 25:** ProcFS & SysFS
  - [ ] /proc entries
  - [ ] /proc/meminfo, /proc/cpuinfo
  - [ ] sysfs hierarchy

### Phase 7: Userland & System Calls

- [ ] **Sprint 26:** C Standard Library (libc)
  - [ ] String functions
  - [ ] printf/scanf
  - [ ] malloc/free

- [ ] **Sprint 27:** System Call Interface
  - [ ] Syscall numbers
  - [ ] int 0x80 entry
  - [ ] Core syscalls

- [ ] **Sprint 28:** User Space Initialization
  - [ ] initramfs
  - [ ] /sbin/init
  - [ ] Shell
  - [ ] Getty/login

- [ ] **Sprint 29:** Essential Utilities
  - [ ] ls, cd, pwd
  - [ ] cat, head, tail
  - [ ] cp, mv, rm

- [ ] **Sprint 30:** Coreutils Implementation
  - [ ] dd, tar
  - [ ] grep, sed
  - [ ] awk, find

- [ ] **Sprint 31:** Shell (Bash-like)
  - [ ] Lexer/parser
  - [ ] Command execution
  - [ ] I/O redirection
  - [ ] Pipes, job control

### Phase 8: Build & Testing

- [ ] **Sprint 32:** Cross-Compiler & Toolchain
  - [ ] x86_64-elf-gcc
  - [ ] LibC (newlib/musl)
  - [ ] GDB

- [ ] **Sprint 33:** Build System Enhancement
  - [ ] Meson/CMake migration
  - [ ] Kconfig
  - [ ] ccache

- [ ] **Sprint 34:** Unit Testing Framework
  - [ ] Test macros
  - [ ] Test runner
  - [ ] Coverage reports

- [ ] **Sprint 35:** Integration Testing
  - [ ] QEMU harness
  - [ ] Boot tests
  - [ ] Filesystem tests

- [ ] **Sprint 36:** Performance Testing
  - [ ] lmbench benchmarks
  - [ ] Memory bandwidth
  - [ ] FS/network benchmarks

### Phase 9: Polish & Documentation

- [ ] **Sprint 37:** System Documentation
  - [ ] Architecture doc
  - [ ] Kernel hacking guide
  - [ ] Man pages

- [ ] **Sprint 38:** Compatibility Layer
  - [ ] POSIX compatibility
  - [ ] Linux emulation
  - [ ] ptrace, core dumps

- [ ] **Sprint 39:** Security Hardening
  - [ ] ASLR
  - [ ] Stack protector
  - [ ] NX enforcement

- [ ] **Sprint 40:** Release Preparation
  - [ ] ISO image
  - [ ] USB image
  - [ ] Versioning
  - [ ] CHANGELOG

---

## Milestone Checklist

### Milestone 1: "Hello World" Kernel (End of Sprint 3)
- [ ] Boots in QEMU
- [ ] Prints to serial/VGA
- [ ] Basic input works

### Milestone 2: Minimal Viable Kernel (End of Sprint 12)
- [ ] Memory management functional
- [ ] Process scheduling works
- [ ] Can run multiple kernel threads

### Milestone 3: Storage Capable (End of Sprint 18)
- [ ] Can read/write disks
- [ ] Filesystems mountable
- [ ] User programs loadable

### Milestone 4: Network Ready (End of Sprint 20)
- [ ] TCP/IP stack works
- [ ] Can create connections
- [ ] Basic web server runs

### Milestone 5: Userland Complete (End of Sprint 31)
- [ ] Shell functional
- [ ] Coreutils work
- [ ] Can compile programs

### Milestone 6: Production Ready (End of Sprint 40)
- [ ] All tests pass
- [ ] Documentation complete
- [ ] ISO bootable
- [ ] Release prepared

---

## Definition of Done (Per Sprint)

A sprint is complete when:
1. All acceptance criteria are met
2. All code compiles without warnings
3. All tests pass
4. Code is reviewed and merged
5. Documentation is updated
6. Demo prepared for review
