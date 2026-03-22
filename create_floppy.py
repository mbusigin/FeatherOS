#!/usr/bin/env python3
"""Create a bootable floppy image for FeatherOS."""

with open('kernel.bin', 'rb') as f:
    kernel = f.read()

img = bytearray(1440 * 1024)
boot = bytearray(512)

# 16-bit real mode code that prints "FeatherOS boot" and halts
code = []
for c in 'FeatherOS boot\r\n':
    code.extend([0xB4, 0x0E, 0xB0, ord(c), 0xCD, 0x10])
code.append(0xF4)  # HLT

for i, b in enumerate(code[:510]):
    boot[i] = b
boot[510], boot[511] = 0x55, 0xAA  # Boot signature

img[:512] = boot
img[512:512+len(kernel)] = kernel

with open('featheros.img', 'wb') as f:
    f.write(img)

print(f"Created featheros.img ({len(kernel)} byte kernel)")
