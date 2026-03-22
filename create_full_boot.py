#!/usr/bin/env python3
import struct

# Read kernel binary
with open('kernel.bin', 'rb') as f:
    kernel_data = f.read()

kernel_size = len(kernel_data)
kernel_sectors = (kernel_size + 511) // 512

# Create complete floppy image with embedded bootloader
# The image will have:
# - Sector 0: 16-bit real mode boot code
# - Sector 1: 32-bit protected mode code  
# - Sector 2+: Kernel binary

img = bytearray(1440 * 1024)
boot_code = bytearray(512)

# ========== 16-bit Boot Code (Sector 0) ==========
# This code runs in real mode and switches to protected mode

code = []

def emit8(b):
    code.append(b & 0xFF)

def emit16(w):
    code.append(w & 0xFF)
    code.append((w >> 8) & 0xFF)

def emit32(d):
    code.append(d & 0xFF)
    code.append((d >> 8) & 0xFF)
    code.append((d >> 16) & 0xFF)
    code.append((d >> 24) & 0xFF)

# Print "Boot" message
msg = "FeatherOS Boot\r\n"
for c in msg:
    emit8(0xB4)
    emit8(0x0E)
    emit8(0xB0)
    emit8(ord(c))
    emit8(0xCD)
    emit8(0x10)

# Disable interrupts
emit8(0xFA)

# Enable A20 line
emit8(0xEB)  # JMP short
emit8(0xFE)  # offset (self-loop placeholder)
# Actually, let's use the standard A20 enable sequence:
code = []
code.extend([0xEB, 0x00])  # JMP short short (filled later)

# A20 enable via Fast A20 gate
emit8(0xE8)  # CALL
emit16(0x0005)  # offset
emit8(0x9C)  # PUSHF
emit8(0xFA)  # CLI
emit8(0xE8)  # CALL
emit16(0x0000)

# Actually, let's just use keyboard controller method
# OUT 0x92, AL - Fast A20 gate
emit8(0xB0)  # MOV AL, imm8
emit8(0x02)  # value for A20
emit8(0xE6)  # OUT imm8, AL
emit8(0x92)

# Now disable interrupts
emit8(0xFA)  # CLI

# Load kernel from sectors 2+ to 0x10000 (we'll copy to 0x100000 in 32-bit mode)
# Using BIOS INT 13h
emit8(0xB4)  # MOV AH, 0
emit8(0x00)
emit8(0xB0)  # MOV AL, 0 (clear)
emit8(0x00)
emit8(0xB6)  # MOV DH, 0
emit8(0x00)
emit8(0xB5)  # MOV CH, 0
emit8(0x00)
emit8(0x88)  # MOV [0x500], DL
emit8(0x16)
emit8(0x00)
emit8(0x05)
emit8(0xCD)  # INT 13h
emit8(0x13)

# Now read kernel sectors into memory at 0x10000
# We'll read 20 sectors (10KB at a time)
for sec in range(0, min(kernel_sectors, 40), 4):
    # Read 4 sectors
    emit8(0xB4)  # MOV AH, 0x02 (read)
    emit8(0x02)
    emit8(0xB0)  # MOV AL, 4
    emit8(0x04)
    emit8(0xB5)  # MOV CH, 0
    emit8(0x00)
    emit8(0xB1)  # MOV CL, sector number
    emit8(2 + sec)  # Start from sector 2
    emit8(0xB6)  # MOV DH, 0
    emit8(0x00)
    emit8(0x8A)  # MOV DL, [0x500]
    emit8(0x16)
    emit8(0x00)
    emit8(0x05)
    emit8(0xBB)  # MOV BX, 0x1000 + (sec * 512)
    emit8((0x1000 + sec * 512) & 0xFF)
    emit8(((0x1000 + sec * 512) >> 8) & 0xFF)
    emit8(0xB7)  # MOV BH, 0x10
    emit8(0x10)
    emit8(0xCD)  # INT 13h
    emit8(0x13)

# Print "." after loading
emit8(0xB4)
emit8(0x0E)
emit8(0xB0)
emit8(ord('.'))
emit8(0xCD)
emit8(0x10)

# Load GDT register
# GDTR will be at 0x5000
emit8(0x0F)  # LGDT [mem]
emit8(0x01)
emit8(0x05)
emit16(0x5000)

# Enable protected mode (CR0.PE = 1)
emit8(0x0F)  # MOV EAX, CR0
emit8(0x20)
emit8(0xC0)
emit8(0x66)  # OR EAX, 1
emit8(0x0C)
emit8(0x01)
emit8(0x00)
emit8(0x00)
emit8(0x00)
emit8(0x0F)  # MOV CR0, EAX
emit8(0x22)
emit8(0xC0)

# Far jump to 32-bit code
emit8(0xEA)  # JMP FAR
emit32(0x00001000)  # Offset (to 32-bit code at 0x10000)
emit16(0x08)  # Selector (code segment)

# Fill rest with zeros
while len(code) < 510:
    emit8(0x00)

# Boot signature
code.append(0x55)
code.append(0xAA)

# Copy code to boot sector
for i in range(min(len(code), 512)):
    boot_code[i] = code[i]

img[0:512] = boot_code

# ========== 32-bit Protected Mode Code (at 0x10000) ==========
# This code copies kernel to 0x100000 and switches to long mode
pm_code = []

def emit32b(d):
    pm_code.append(d & 0xFF)
    pm_code.append((d >> 8) & 0xFF)
    pm_code.append((d >> 16) & 0xFF)
    pm_code.append((d >> 24) & 0xFF)

# Set up segments
emit8(0x66)  # MOV AX, 0x10
emit8(0xB8)
emit32b(0x10)
emit8(0x8E)  # MOV DS, AX
emit8(0xD8)
emit8(0x8E)  # MOV ES, AX
emit8(0xC0)
emit8(0x8E)  # MOV FS, AX
emit8(0xE0)
emit8(0x8E)  # MOV GS, AX
emit8(0xE8)
emit8(0x8E)  # MOV SS, AX
emit8(0xD0)

# Set up stack
emit8(0x66)  # MOV ESP, 0x200000
emit8(0xBC)
emit32b(0x200000)

# Copy kernel from 0x10000 to 0x100000
# Using REP MOVSB
emit8(0x66)  # MOV ESI, 0x10000
emit8(0xBE)
emit32b(0x10000)
emit8(0x66)  # MOV EDI, 0x100000
emit8(0xBF)
emit32b(0x100000)
emit8(0x66)  # MOV ECX, kernel_size
emit8(0xB9)
emit32b(kernel_size)
emit8(0xF3)  # REP MOVSB
emit8(0xA5)

# Print "X" for 32-bit mode
emit8(0x66)
emit8(0xB0)  # MOV AL, 'X'
emit8(0x58)
emit8(0x66)  # OUT 0x3F8, AL
emit8(0xE6)
emit8(0xF8)

# Enable PAE (CR4.PAE = 5)
emit8(0x0F)  # MOV EAX, CR4
emit8(0x20)
emit8(0xC0)
emit8(0x66)  # OR EAX, 0x20
emit8(0x0C)
emit8(0x20)
emit8(0x00)
emit8(0x00)
emit8(0x00)
emit8(0x0F)  # MOV CR4, EAX
emit8(0x22)
emit8(0xE0)

# Load page tables at 0x20000 (we'll create them inline)
# PML4 at 0x20000
# PDP at 0x21000  
# PD at 0x22000

# PML4 entry
emit8(0x66)  # MOV DWORD [0x20000], 0x21003
emit8(0xC7)
emit8(0x05)
emit32b(0x20000)
emit32b(0x21003)

# PDP entry
emit8(0x66)
emit8(0xC7)
emit8(0x05)
emit32b(0x21000)
emit32b(0x22083)  # 0x83 = present, writable, 4MB page

# PD entry (0x22000) - already set by PDP

# Load CR3
emit8(0x0F)  # MOV EAX, 0x20000
emit8(0x20)
emit8(0xC0)
emit8(0x66)
emit8(0xB8)
emit32b(0x20000)
emit8(0x0F)  # MOV CR3, EAX
emit8(0x22)
emit8(0xD8)

# Enable long mode in EFER
emit8(0x0F)  # MOV ECX, 0xC0000080 (EFER)
emit8(0x32)
emit8(0x66)
emit8(0xB1)
emit32b(0xC0000080)
emit8(0x0F)  # RDMSR
emit8(0x30)
emit8(0x66)  # OR EAX, 0x100 (LME)
emit8(0x0C)
emit8(0x00)
emit8(0x01)
emit8(0x00)
emit8(0x00)
emit8(0x0F)  # WRMSR
emit8(0x30)

# Enable paging (CR0.PG)
emit8(0x0F)  # MOV EAX, CR0
emit8(0x20)
emit8(0xC0)
emit8(0x66)  # OR EAX, 0x80000000 (PG)
emit8(0x0C)
emit32b(0x80000000)
emit8(0x0F)  # MOV CR0, EAX
emit8(0x22)
emit8(0xC0)

# Jump to 64-bit mode
emit8(0xEA)  # JMP FAR
emit32b(0x00000000)  # Offset (filled with 0x100000)
emit16(0x08)  # Selector

# Pad to fill sector
while len(pm_code) < 512:
    pm_code.append(0x90)

img[0x10000:0x10000 + 512] = bytes(pm_code)

# ========== Page Tables (at 0x20000, 0x21000, 0x22000) ==========
# PML4 (0x20000) - single entry pointing to PDP
pml4 = bytearray(4096)
struct.pack_into('<Q', pml4, 0, 0x21003)  # Present, writable, PDP at 0x21000
img[0x20000:0x20000 + 4096] = pml4

# PDP (0x21000) - single entry pointing to PD
pdpt = bytearray(4096)
struct.pack_into('<Q', pdpt, 0, 0x22083)  # Present, writable, 4MB page
img[0x21000:0x21000 + 4096] = pdpt

# PD (0x22000) - maps first 1GB to 0x100000 with 4KB pages
pd = bytearray(4096)
# First entry: 0x0000000000000003 (identity map first 2MB)
struct.pack_into('<Q', pd, 0, 0x0000000000000083)  # Present, writable, 2MB page at 0x0
# Map 0x100000 to 0x100000 (identity for kernel)
# Use 4KB pages for more control
pd_bytes = bytearray(4096)
for i in range(512):  # 512 * 2MB = 1GB
    struct.pack_into('<Q', pd_bytes, i * 8, 0x0000000000000083 | (i * 0x200000))
img[0x22000:0x22000 + 4096] = pd_bytes

# ========== Write Kernel ==========
kernel_offset = 512 * 2
img[kernel_offset:kernel_offset + kernel_size] = kernel_data

# Also write at 0x10000 (where boot code loads it)
img[0x10000 + 512:0x10000 + 512 + kernel_size] = kernel_data

# ========== Write GDT at 0x5000 ==========
gdt = bytearray()
# Null descriptor
gdt.extend([0] * 8)
# Kernel code (base=0, limit=0xFFFFF, type=0x9A)
gdt.extend(struct.pack('<HHHHH', 0xFFFF, 0x0000, 0x9A00, 0x00CF, 0x0089))
# Kernel data
gdt.extend(struct.pack('<HHHHH', 0xFFFF, 0x0000, 0x9200, 0x00CF, 0x0089))
# TSS (placeholder)
gdt.extend([0] * 16)

while len(gdt) < 256:
    gdt.append(0)

img[0x5000:0x5000 + len(gdt)] = gdt

with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img")
print(f"  Kernel size: {kernel_size} bytes")
print(f"  Boot sector: 0x0-0x200")
print(f"  32-bit code: 0x10000-0x10200")
print(f"  PML4: 0x20000")
print(f"  PDP: 0x21000")
print(f"  PD: 0x22000")
print(f"  GDT: 0x5000")
print(f"  Kernel at: 0x{kernel_offset:X}")
