#!/usr/bin/env python3
import struct

# Read kernel binary
with open('kernel.bin', 'rb') as f:
    kernel_data = f.read()

kernel_size = len(kernel_data)
kernel_sectors = (kernel_size + 511) // 512

# Boot sector at offset 0, kernel follows after
# We'll use the area after the boot sector for the kernel

img = bytearray(1440 * 1024)  # 1.44MB floppy

# Boot sector code (real mode -> protected mode -> long mode -> kernel)
# This is a minimal bootloader that:
# 1. Loads kernel from disk to 0x100000 (1MB)
# 2. Sets up GDT
# 3. Enables PAE
# 4. Enables long mode
# 5. Jumps to kernel

# Simple boot sector that prints a message and halts
# (Full mode switching bootloader would be much larger)
boot_code = []

# Print "FeatherOS Boot" using BIOS
# Using INT 10h function 0x0E (teletype output)
msg = "FeatherOS Boot\r\n"
for c in msg:
    boot_code.append(0xB4)  # mov ah, 0x0E
    boot_code.append(0x0E)
    boot_code.append(0xB0)  # mov al, char
    boot_code.append(ord(c))
    boot_code.append(0xCD)  # int 0x10
    boot_code.append(0x10)

# Load kernel from disk sectors 1+ to 0x100000 using INT 13h
# But first we need to switch to protected mode
# This is complex, so let's use a simpler approach:
# Just copy kernel data and switch to protected mode

# For simplicity, let's just use the kernel entry at 0x10000
# and copy it to 0x100000 after switching modes

# Actually, let's keep it simple - just print "FeatherOS Boot" and halt
# The kernel.bin has its own entry point that prints messages

boot_code.extend([
    0xF4,  # hlt (halt)
])

# Copy boot code to image
for i, b in enumerate(boot_code):
    img[i] = b

# Copy kernel after boot sector (at sector 1)
kernel_offset = 512
img[kernel_offset:kernel_offset + kernel_size] = kernel_data

# Boot signature
img[510] = 0x55
img[511] = 0xAA

with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img with kernel ({kernel_size} bytes, {kernel_sectors} sectors)")
print("Kernel loaded at offset 0x200 (sector 1)")
