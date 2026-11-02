; BingoOS entry — точка входа ядра (32-bit)
format binary
use32
org 0x10000

_start:
    cli
    cld

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, 0x90000

    call kmain

.halt:
    cli
    hlt
    jmp .halt

include 'kernel\main.asm'
