#!/usr/bin/env python3
import struct

def emit_byte(code, b):
    code.append(b & 0xFF)

def emit_word(code, w):
    code.append(w & 0xFF)
    code.append((w >> 8) & 0xFF)

def emit_dword(code, d):
    code.append(d & 0xFF)
    code.append((d >> 8) & 0xFF)
    code.append((d >> 16) & 0xFF)
    code.append((d >> 24) & 0xFF)

def emit_qword(code, q):
    for i in range(8):
        code.append((q >> (i * 8)) & 0xFF)

# Read kernel binary
with open('kernel.bin', 'rb') as f:
    kernel_data = f.read()

kernel_size = len(kernel_data)
kernel_sectors = (kernel_size + 511) // 512

# Create boot sector code (512 bytes)
boot_code = bytearray(512)

code = []

# =========== REAL MODE SETUP ===========
# Print "Loading..." message
msg_load = "Loading FeatherOS...\r\n"
for c in msg_load:
    emit_byte(code, 0xB4)  # mov ah, 0x0E
    emit_byte(code, 0x0E)
    emit_byte(code, 0xB0)  # mov al, char
    emit_byte(code, ord(c))
    emit_byte(code, 0xCD)  # int 0x10
    emit_byte(code, 0x10)

# Reset floppy disk controller
emit_byte(code, 0xB4)  # mov ah, 0
emit_byte(code, 0x00)
emit_byte(code, 0xB0)  # mov dl, 0 (floppy)
emit_byte(code, 0x00)
emit_byte(code, 0xCD)  # int 0x13
emit_byte(code, 0x13)

# Load kernel from floppy to 0x100000 using BIOS INT 13h
# We'll use CHS addressing for simplicity
# Sector 1, Head 0, Drive 0

# For reading we need to switch to protected mode first
# So let's put the disk read + mode switch code in the next 32KB

# Actually, let's use a different approach:
# Copy the kernel data directly into the image and load it from there
# Use BIOS INT 13h function 42h (extended read) if available

# For simplicity, let's use the traditional CHS read:
# AH = 0x02 (read sectors)
# AL = number of sectors
# CH = cylinder
# CL = sector
# DH = head
# DL = drive
# ES:BX = buffer

# We'll read 2KB at a time (4 sectors)
# Kernel is at sector 1+ of the floppy

# Set ES:BX to 0x0000:0x1000 (temporary buffer in low memory)
emit_byte(code, 0xBE)  # mov si, offset
emit_byte(code, 0x00)  # low byte
emit_byte(code, 0x10)  # high byte (0x1000)

# We'll do this in chunks - for simplicity, just load first 4 sectors

# DL = drive number (will be preserved by BIOS)
# Actually, BIOS passes drive number in DL - we need to save it

# Save DL to a known location
emit_byte(code, 0x88)  # mov [0x500], dl
emit_byte(code, 0x16)
emit_byte(code, 0x00)
emit_byte(code, 0x05)

# Read sectors using BIOS
# AH = 0x02
# AL = 4 (sectors to read)
# CH = 0 (cylinder)
# CL = 2 (start from sector 2, sector 1 is boot sector)
# DH = 0 (head)
# DL = drive number (from 0x500)
emit_byte(code, 0xB4)  # mov ah, 0x02
emit_byte(code, 0x02)
emit_byte(code, 0xB0)  # mov al, 4
emit_byte(code, 0x04)
emit_byte(code, 0xB5)  # mov ch, 0
emit_byte(code, 0x00)
emit_byte(code, 0xB1)  # mov cl, 2
emit_byte(code, 0x02)
emit_byte(code, 0xB6)  # mov dh, 0
emit_byte(code, 0x00)
emit_byte(code, 0x8A)  # mov dl, [0x500]
emit_byte(code, 0x16)
emit_byte(code, 0x00)
emit_byte(code, 0x05)
emit_byte(code, 0xBB)  # mov bx, 0x1000
emit_byte(code, 0x00)
emit_byte(code, 0x10)
emit_byte(code, 0xCD)  # int 0x13
emit_byte(code, 0x13)

# Check carry flag for error
emit_byte(code, 0x72)  # JC error
emit_byte(code, 0xF4)  # offset to error

# Copy loaded data to 0x100000
# We'll do this in protected mode

# Actually, for simplicity, let's just halt after loading
# and the kernel will be executed by jumping to its entry

# Print "OK"
msg_ok = "\r\nOK!\r\n"
for c in msg_ok:
    emit_byte(code, 0xB4)
    emit_byte(code, 0x0E)
    emit_byte(code, 0xB0)
    emit_byte(code, ord(c))
    emit_byte(code, 0xCD)
    emit_byte(code, 0x10)

# Halt
emit_byte(code, 0xF4)

# Error label
# emit_byte(code, 0xE9)  # JMP short error
# emit_byte(code, 0x00)
# emit_byte(code, 0x00)

# Fill rest with NOPs
while len(code) < 510:
    emit_byte(code, 0x90)  # NOP

# Boot signature
boot_code[510] = 0x55
boot_code[511] = 0xAA

# Copy code to boot sector
for i, b in enumerate(code):
    boot_code[i] = b

print(f"Boot code size: {len(code)} bytes")

# Create floppy image
img = bytearray(1440 * 1024)

# Write boot sector
img[0:512] = boot_code

# Write kernel starting at sector 2
kernel_offset = 512 * 2  # Sector 2
img[kernel_offset:kernel_offset + kernel_size] = kernel_data

# Also write a copy at 0x1000 (where boot code loads it)
img[0x1000:0x1000 + kernel_size] = kernel_data

with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img")
print(f"  Kernel size: {kernel_size} bytes")
print(f"  Kernel at offset 0x{kernel_offset:X} (sector 2)")
print(f"  Also at 0x1000 (where boot code loads)")
