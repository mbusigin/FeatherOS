# Sprint 4: Integrate Userland Build

**Epic:** FeatherOS User Space Shell  
**Phase:** Phase 4  
**Status:** TODO  
**Priority:** P1 (High)  
**Estimated Duration:** 2 days  

## Goal
Fix `make userland` and `make run` to actually work.

## Background
Makefile has userland targets but they're not integrated into the build chain. create_floppy.py only prints "FeatherOS boot" and halts.

## Tasks

### 4.1 Fix userland compilation (Day 1)
```makefile
# In Makefile, fix userland target:
userland:
	mkdir -p $(BUILD_DIR)/initramfs/bin $(BUILD_DIR)/initramfs/sbin
	$(CC) $(CFLAGS) -nostdlib -fno-pie -e _start \
		-o $(BUILD_DIR)/initramfs/shell \
		$(SRC_DIR)/userland/shell/shell.c
```

### 4.2 Create initramfs (Day 1)
```bash
# Create directory structure
mkdir -p build/initramfs/bin build/initramfs/sbin
# Copy binaries
cp build/initramfs/shell build/initramfs/bin/sh
cp build/initramfs/shell build/initramfs/sbin/init
# Create cpio archive
cd build/initramfs && find . -type f | cpio -o -H newc > ../initramfs.cpio
```

### 4.3 Fix create_floppy.py (Day 2)
- Copy kernel.bin to floppy at correct offset (0x2000)
- Set up proper MBR boot signature
- Ensure BIOS loads kernel at 0x100000

### 4.4 Update Makefile run target (Day 2)
```makefile
run: userland floppy
	qemu-system-x86_64 -fda featheros.img -m 256M \
		-serial file:serial.log -no-reboot
```

## Deliverables
- `make userland` produces shell binary
- `make run` launches QEMU
- Serial log shows boot messages

## Acceptance Criteria
```bash
# Test: Full build
cd project && make clean && make userland 2>&1 | tail -10
# Should show: "Userland build complete"
ls -la build/initramfs/shell  # Binary exists
```

## Dependencies
- shell.c (existing)
- Makefile (existing)

## Notes
- Use -nostdlib to avoid linking standard library
- Use -e _start to set entry point
- Test build on clean checkout
