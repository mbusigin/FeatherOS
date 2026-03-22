#!/usr/bin/env python3
import struct

# Read kernel binary
with open('kernel.bin', 'rb') as f:
    kernel_data = f.read()

kernel_size = len(kernel_data)

# Create a minimal floppy image with:
# - Boot sector that prints "FeatherOS boot" and halts
# - Kernel binary after the boot sector

img = bytearray(1440 * 1024)

# Boot sector code (512 bytes)
boot = bytearray(512)

# 16-bit real mode code that prints "FeatherOS boot" and halts
code = []

# Print "F"
code.extend([0xB4, 0x0E, 0xB0, ord('F'), 0xCD, 0x10])
# Print "e"
code.extend([0xB4, 0x0E, 0xB0, ord('e'), 0xCD, 0x10])
# Print "a"
code.extend([0xB4, 0x0E, 0xB0, ord('a'), 0xCD, 0x10])
# Print "t"
code.extend([0xB4, 0x0E, 0xB0, ord('t'), 0xCD, 0x10])
# Print "h"
code.extend([0xB4, 0x0E, 0xB0, ord('h'), 0xCD, 0x10])
# Print "e"
code.extend([0xB4, 0x0E, 0xB0, ord('e'), 0xCD, 0x10])
# Print "r"
code.extend([0xB4, 0x0E, 0xB0, ord('r'), 0xCD, 0x10])
# Print "O"
code.extend([0xB4, 0x0E, 0xB0, ord('O'), 0xCD, 0x10])
# Print "S"
code.extend([0xB4, 0x0E, 0xB0, ord('S'), 0xCD, 0x10])
# Print " "
code.extend([0xB4, 0x0E, 0xB0, ord(' '), 0xCD, 0x10])
# Print "b"
code.extend([0xB4, 0x0E, 0xB0, ord('b'), 0xCD, 0x10])
# Print "o"
code.extend([0xB4, 0x0E, 0xB0, ord('o'), 0xCD, 0x10])
# Print "o"
code.extend([0xB4, 0x0E, 0xB0, ord('o'), 0xCD, 0x10])
# Print "t"
code.extend([0xB4, 0x0E, 0xB0, ord('t'), 0xCD, 0x10])
# Print "\r"
code.extend([0xB4, 0x0E, 0xB0, 0x0D, 0xCD, 0x10])
# Print "\n"
code.extend([0xB4, 0x0E, 0xB0, 0x0A, 0xCD, 0x10])

# Halt
code.append(0xF4)

# Copy to boot sector
for i, b in enumerate(code[:510]):
    boot[i] = b

# Boot signature
boot[510] = 0x55
boot[511] = 0xAA

# Write boot sector
img[0:512] = boot

# Write kernel after boot sector
img[512:512 + kernel_size] = kernel_data

with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img: boot sector + kernel ({kernel_size} bytes)")
