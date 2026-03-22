#!/usr/bin/env python3
import struct

# Read kernel binary
with open('kernel.bin', 'rb') as f:
    kernel_data = f.read()

# Create a simple bootable floppy image
# 1.44MB floppy
img_size = 1440 * 1024
img = bytearray(img_size)

# Boot sector at offset 0
# Simple real-mode boot code that just halts after printing
boot_code = bytearray([
    # Print "FeatherOS" using BIOS INT 10h
    0xB4, 0x0E,              # mov ah, 0x0E (teletype)
    0xB0, ord('F'),          # mov al, 'F'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('e'),          # mov al, 'e'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('a'),          # mov al, 'a'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('t'),          # mov al, 't'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('h'),          # mov al, 'h'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('e'),          # mov al, 'e'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('r'),          # mov al, 'r'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('O'),          # mov al, 'O'
    0xCD, 0x10,              # int 0x10
    0xB0, ord('S'),          # mov al, 'S'
    0xCD, 0x10,              # int 0x10
    0xB0, 0x0D,              # mov al, '\r'
    0xCD, 0x10,              # int 0x10
    0xB0, 0x0A,              # mov al, '\n'
    0xCD, 0x10,              # int 0x10
    0xF4,                    # hlt
])

# Copy boot code
for i, b in enumerate(boot_code):
    img[i] = b

# Boot signature
img[510] = 0x55
img[511] = 0xAA

# Write image
with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img ({img_size} bytes)")
