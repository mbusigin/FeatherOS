# FeatherOS - Quick Reference

## Project Overview
- **Name:** FeatherOS
- **Type:** Educational/Production-ready x86-64 Operating System
- **Architecture:** x86-64 (AMD64)
- **Kernel:** Monolithic
- **Language:** C (kernel), Assembly (boot/arch-specific)
- **Estimated Duration:** 40 sprints (~80 weeks)

## Key Documents
| Document | Description |
|----------|-------------|
| `EPIC.md` | Full project plan with all 40 sprints |
| `SPRINT_CHECKLIST.md` | Sprint-by-sprint tracking checklist |
| `QUICKSTART.md` | This file - quick reference |

## Directory Structure
```
/project/
├── FeatherOS/          # Main kernel source
├── docs/               # Sphinx documentation
├── scripts/            # Build/run scripts
└── tests/              # Test suites
```

## Quick Build Commands
```bash
# Full build
make all

# Clean build
make clean

# Run in QEMU
make run

# Run tests
make test

# Build with debug
make debug
```

## Architecture Overview
```
┌─────────────────────────────────────────┐
│            User Space                    │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐ │
│  │  libc   │  │  shell  │  │  apps   │ │
│  └────┬────┘  └────┬────┘  └────┬────┘ │
│       └───────────┼─────────────┘      │
│                   │ syscall             │
├───────────────────┼─────────────────────┤
│            Kernel │                      │
│  ┌────────────────┼────────────────────┐ │
│  │    VFS Layer   │                    │ │
│  ├────────┬──────┼──────┬─────────────┤ │
│  │  Proc  │  Net │  MM  │  Scheduler  │ │
│  ├────────┴──────┴──────┴─────────────┤ │
│  │         Device Drivers             │ │
│  ├────────────────────────────────────┤ │
│  │     x86_64 Architecture Code       │ │
│  ├────────────────────────────────────┤ │
│  │     Bootloader (GRUB2 multiboot2)  │ │
│  └────────────────────────────────────┘ │
└─────────────────────────────────────────┘
```

## Key Milestones
| Milestone | Sprints | Goal |
|-----------|---------|------|
| M1: Hello Kernel | 1-3 | Boot, print, basic I/O |
| M2: Minimal Viable | 4-12 | Memory, processes, scheduling |
| M3: Storage Ready | 13-18 | Drivers, block I/O, FS |
| M4: Network Ready | 19-20 | Full TCP/IP stack |
| M5: Userland | 21-31 | Shell, utilities, libc |
| M6: Production | 32-40 | Testing, docs, release |

## System Calls (Core)
| Number | Name | Description |
|--------|------|-------------|
| 0 | read | Read from file descriptor |
| 1 | write | Write to file descriptor |
| 2 | open | Open file |
| 3 | close | Close file descriptor |
| 57 | fork | Create child process |
| 59 | execve | Execute program |
| 60 | exit | Terminate process |
| 231 | exit_group | Exit all threads |
| 9 | mmap | Map memory |
| 11 | munmap | Unmap memory |
| 12 | brk | Change data segment size |

## Target Hardware
- **CPU:** x86-64 (AMD64), SVM/VMX for virtualization
- **RAM:** 256MB minimum, 1GB recommended
- **Storage:** SATA/ATA, USB
- **Network:** Intel e1000, Realtek 8139, virtio-net
- **Display:** VGA, VESA 2.0+

## Performance Targets
| Metric | Target |
|--------|--------|
| Boot time (QEMU) | < 10 seconds |
| Context switch | < 5 μs |
| Syscall latency | < 1 μs |
| FS throughput | > 50 MB/s |
| Memory bandwidth | > 5 GB/s |

## Testing Strategy
- Unit tests for all data structures
- Integration tests via QEMU
- Boot tests (success/fail detection)
- Benchmark suite (lmbench-style)
- CI/CD on every commit

## Contributing
1. Pick an unstarted sprint from `SPRINT_CHECKLIST.md`
2. Create feature branch: `sprint-XX-feature-name`
3. Implement tasks and tests
4. Ensure all tests pass
5. Submit PR with review
6. Squash and merge on approval

## Resources
- OS Dev Wiki: https://wiki.osdev.org
- Intel SDM: https://software.intel.com/software-sdm
- Linux Kernel Source: github.com/torvalds/linux
- xv6: github.com/mit-pdos/xv6-public

---

*Last Updated: Sprint 0 (Planning Phase Complete)*
