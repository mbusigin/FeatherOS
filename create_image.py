#!/usr/bin/env python3

# Create a minimal floppy image
boot_code = bytearray(512)
boot_code[0:3] = b'\xEB\xFE\x90'  # jmp short $
boot_code[3:13] = b'FeatherOS'
boot_code[510:512] = b'\x55\xAA'

with open('featheros.img', 'wb') as f:
    f.write(boot_code)
    f.write(b'\x00' * (1440 * 1024 - 512))

print("Created featheros.img")
