# FeatherOS

A modern, POSIX-compatible operating system kernel for x86_64.

## Build Status

![Build](https://github.com/featheros/featheros/workflows/CI/badge.svg)

## Overview

FeatherOS is an educational operating system project implementing a full OS from scratch. The goal is to create a production-ready monolithic kernel with:

- Modern x86_64 architecture support
- Virtual file system (VFS) layer
- Paging-based memory management (buddy + slab allocators)
- Preemptive multiprocess scheduling (CFS)
- Comprehensive device drivers
- POSIX-compatible userland

## Building

### Prerequisites

```bash
# macOS (with Homebrew)
brew install x86_64-elf-gcc x86_64-elf-binutils

# Linux (Debian/Ubuntu)
apt-get install gcc-x86-64-elf binutils-x86-64-elf grub-common xorriso

# Or build cross-compiler from source
# See scripts/build-toolchain.sh
```

### Build Commands

```bash
# Full build (debug mode)
make all

# Release build
make release

# Create bootable ISO
make iso

# Run in QEMU
make run

# Clean build
make clean
```

## Running

### QEMU (Recommended for development)

```bash
make run          # Interactive mode
make run-qemu     # With serial logging
make run-gdb      # With GDB debugging
```

### Real Hardware

1. Create bootable media:
   ```bash
   make iso
   # or
   make usb  # if target is USB
   ```

2. Flash to USB/CD and boot.

## Project Structure

```
FeatherOS/
├── boot/          # Bootloader and boot code
├── kernel/        # Kernel source
│   ├── arch/      # Architecture-specific (x86_64)
│   ├── mm/        # Memory management
│   ├── sched/     # Scheduler
│   ├── fs/        # Filesystem
│   ├── net/       # Networking
│   ├── drivers/   # Device drivers
│   └── sync/      # Synchronization primitives
├── include/       # Header files
├── lib/           # Standard library
├── userland/      # Userspace programs
└── tests/         # Test suites
```

## Development

### Prerequisites

- x86_64-elf-gcc (cross-compiler)
- GNU binutils (x86_64-elf)
- GRUB2 (for ISO creation)
- QEMU (for emulation)
- Python 3 (for scripts)

### Building Toolchain

```bash
# Automated build
./scripts/build-toolchain.sh
```

## Testing

```bash
make test              # Run all tests
make test-unit         # Unit tests
make test-integration  # Integration tests
make test-qemu         # QEMU boot test
```

## Documentation

- [Architecture Documentation](docs/architecture.md)
- [Kernel Hacking Guide](docs/hacking.md)
- [System Call Interface](docs/syscalls.md)
- [Driver Development](docs/drivers.md)

## Contributing

1. Pick a sprint from the backlog
2. Create feature branch: `git checkout -b sprint-XX-feature`
3. Implement and test
4. Submit pull request

## License

MIT License - See LICENSE file.

## Acknowledgments

- [OSDev Wiki](https://wiki.osdev.org)
- [Linux Kernel Source](https://github.com/torvalds/linux)
- [xv6](https://github.com/mit-pdos/xv6-public)
- [Writing a Simple Operating System from Scratch](http://www.cs.bham.ac.uk/~exr/lectures/opsys/10_11/lectures/os-dev.pdf)
