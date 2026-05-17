BITS 64
GLOBAL _start

SECTION .text
_start:
    ; syscall 1: write(fd=1, buf=msg, len=len)
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel msg]
    mov rdx, len
    int 0x80

    ; syscall 0: exit(status=0)
    mov rax, 0
    xor rdi, rdi
    int 0x80

    ; should not reach here
    jmp $

SECTION .rodata
msg: db "Hello from userspace!", 10
len: equ $ - msg
