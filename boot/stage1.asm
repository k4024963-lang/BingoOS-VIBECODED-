; BingoOS FAT12 boot sector — BIOS EDD stage1
format binary
org 0x7C00
jmp short boot_start
nop

db 'BINGOOS '              ; OEM
dw 512                     ; bytes/sector
db 1                       ; sectors/cluster
dw 66                      ; reserved: boot + stage2 + 64 kernel sectors
db 2                       ; FAT copies
dw 224                     ; root entries
dw 2880                    ; total sectors
db 0xF0                    ; media
dw 9                       ; sectors/FAT
dw 18                      ; sectors/track (BPB geometry)
dw 2                       ; heads
dd 0                       ; hidden sectors
dd 0                       ; total sectors 32-bit
db 0                       ; drive number
db 0                       ; reserved
db 0x29                    ; extended boot signature
dd 0x20261124              ; volume serial
db 'BINGOOS    '           ; label
db 'FAT12   '              ; filesystem type

boot_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti
    mov [0x7E00], dl

    ; EDD read LBA 1 (stage2) to 0000:8000.
    mov si, stage2_dap
    mov dl, [0x7E00]
    mov ah, 0x42
    int 0x13
    jc disk_error
    jmp 0x0000:0x8000

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

stage2_dap:
    db 0x10, 0
    dw 1
    dw 0x8000
    dw 0
    dq 1
msg_disk_error db 'BingoOS: stage2 read error', 0

times 510-($-$$) db 0
dw 0xAA55
