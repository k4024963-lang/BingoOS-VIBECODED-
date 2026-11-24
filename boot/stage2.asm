; BingoOS stage2 — BIOS EDD kernel load, then 32-bit protected mode
format binary
org 0x8000
start:
    cli
    mov si, disk_address_packet
    mov dl, [0x7E00]
    mov ah, 0x42
    int 0x13
    jc disk_error
    in al, 0x92
    or al, 2
    out 0x92, al
    lgdt [gdt_descriptor]
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    jmp 0x08:protected_mode

disk_error:
    mov si, msg_disk_error
    call print16
.halt:
    cli
    hlt
    jmp .halt
print16:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print16
.done:
    ret
msg_disk_error db 'BingoOS: EDD kernel read error', 0

align 4
disk_address_packet:
    db 0x10, 0
    dw 32
    dw 0x0000
    dw 0x1000
    dq 2

align 8
gdt_start:
    dq 0
gdt_code:
    dw 0xFFFF, 0x0000
    db 0x00, 10011010b, 11001111b, 0x00
gdt_data:
    dw 0xFFFF, 0x0000
    db 0x00, 10010010b, 11001111b, 0x00
gdt_end:
gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

use32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    jmp 0x10000
times 512-($-$$) db 0
