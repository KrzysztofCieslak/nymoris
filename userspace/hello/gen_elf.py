#!/usr/bin/env python3
"""Generate a minimal x86_64 ELF executable that prints 'Hello from userspace!' and exits."""

import struct

# The code uses RIP-relative addressing to reference the message string.
# Message is placed right after the code.
# Layout at 0x400000:
#   0x0000 - 0x002f: code (48 bytes)
#   0x0030 - 0x0045: message string (22 bytes)

# Code:
#   mov rax, 1           ; syscall number (write)
#   mov rdi, 1           ; fd = stdout
#   lea rsi, [rip + msg] ; msg is at code_end, which is 0x30 bytes from start
#                          lea is 7 bytes, so offset = 0x30 - (0x0e + 7) = 0x17
#                          Wait, let me recalculate.
#   mov rdx, len         ; message length
#   int 0x80             ; syscall
#   mov rax, 0           ; syscall number (exit)
#   xor rdi, rdi         ; status = 0
#   int 0x80             ; syscall

# Let me write the code more carefully:
# Offset 0x00: 48 c7 c0 01 00 00 00     mov rax, 1          (7 bytes)
# Offset 0x07: 48 c7 c7 01 00 00 00     mov rdi, 1          (7 bytes)
# Offset 0x0e: 48 8d 35 16 00 00 00     lea rsi, [rip+0x16] (7 bytes)
#               The instruction is at 0x0e, length 7, so next is at 0x15.
#               Message is at 0x30. Offset = 0x30 - 0x15 = 0x1b = 27
# Offset 0x15: 48 c7 c2 16 00 00 00     mov rdx, 0x16       (7 bytes)
# Offset 0x1c: cd 80                     int 0x80            (2 bytes)
# Offset 0x1e: 48 c7 c0 00 00 00 00     mov rax, 0          (7 bytes)
# Offset 0x25: 48 31 ff                 xor rdi, rdi        (3 bytes)
# Offset 0x28: cd 80                     int 0x80            (2 bytes)
# Offset 0x2a: eb fe                     jmp $               (2 bytes)  ; infinite loop (fallback)
# Total code: 0x2c = 44 bytes
# Message at 0x2c: "Hello from userspace!\n" = 22 bytes = 0x16
# Total: 0x42 = 66 bytes

code = bytearray([
    0x48, 0xc7, 0xc0, 0x01, 0x00, 0x00, 0x00,  # mov rax, 1
    0x48, 0xc7, 0xc7, 0x01, 0x00, 0x00, 0x00,  # mov rdi, 1
    0x48, 0x8d, 0x35, 0x00, 0x00, 0x00, 0x00,  # lea rsi, [rip+offset] - placeholder
    0x48, 0xc7, 0xc2, 0x16, 0x00, 0x00, 0x00,  # mov rdx, 22
    0xcd, 0x80,                                    # int 0x80
    0x48, 0xc7, 0xc0, 0x00, 0x00, 0x00, 0x00,  # mov rax, 0
    0x48, 0x31, 0xff,                              # xor rdi, rdi
    0xcd, 0x80,                                    # int 0x80
    0xeb, 0xfe,                                    # jmp $
])

# Fix up the lea offset
# lea instruction is at offset 0x0e, length 7
# Next instruction is at 0x15
# Message is at offset len(code) = 0x2c
offset = len(code) - (0x0e + 7)
code[0x11:0x15] = struct.pack("<i", offset)

msg = b"Hello from userspace!\n"

header_size = 64 + 56  # ELF header + program header
total_size = header_size + len(code) + len(msg)
entry_point = 0x400000 + header_size

# ELF64 header (64 bytes)
elf = bytearray(64)
elf[0:4]   = b'\x7fELF'                           # magic
elf[4]     = 2                                   # 64-bit
elf[5]     = 1                                   # little endian
elf[6]     = 1                                   # ELF version
elf[7]     = 0                                   # OS/ABI
elf[8:16]  = bytes(8)                            # padding
elf[16:18] = struct.pack('<H', 2)                # e_type = ET_EXEC
elf[18:20] = struct.pack('<H', 62)               # e_machine = EM_X86_64
elf[20:24] = struct.pack('<I', 1)                # e_version
elf[24:32] = struct.pack('<Q', entry_point)      # e_entry
elf[32:40] = struct.pack('<Q', 64)               # e_phoff
elf[40:48] = struct.pack('<Q', 0)                # e_shoff
elf[48:52] = struct.pack('<I', 0)                # e_flags
elf[52:54] = struct.pack('<H', 64)               # e_ehsize
elf[54:56] = struct.pack('<H', 56)               # e_phentsize
elf[56:58] = struct.pack('<H', 1)                # e_phnum
elf[58:60] = struct.pack('<H', 64)               # e_shentsize
elf[60:62] = struct.pack('<H', 0)                # e_shnum
elf[62:64] = struct.pack('<H', 0)                # e_shstrndx

# Program header (56 bytes)
ph = bytearray(56)
ph[0:4]   = struct.pack('<I', 1)                 # p_type = PT_LOAD
ph[4:8]   = struct.pack('<I', 5)                 # p_flags = PF_X | PF_R
ph[8:16]  = struct.pack('<Q', 0)                 # p_offset
ph[16:24] = struct.pack('<Q', 0x400000)          # p_vaddr
ph[24:32] = struct.pack('<Q', 0x400000)          # p_paddr
ph[32:40] = struct.pack('<Q', total_size)        # p_filesz
ph[40:48] = struct.pack('<Q', total_size)        # p_memsz
ph[48:56] = struct.pack('<Q', 0x1000)            # p_align

data = bytes(elf) + bytes(ph) + bytes(code) + msg

with open('hello.elf', 'wb') as f:
    f.write(data)

print(f"Generated hello.elf ({len(data)} bytes)")
print(f"  Entry point: {entry_point:#x}")
print(f"  Code size: {len(code)} bytes")
print(f"  Message size: {len(msg)} bytes")
