; BingoOS stage1 — FAT12 boot sector (BIOS)
; Sector layout: LBA 0 boot, LBA 1 stage2, LBA 2..33 kernel.
format binary
org 0x7C00

jmp short boot_start
nop

; FAT12 BIOS Parameter Block (1.44 MiB, 1.44 MB floppy geometry)
db 'BINGOOS '              ; OEM name (8)
dw 512                     ; bytes per sector
db 1                       ; sectors per cluster
dw 34                      ; reserved sectors: stage1 + stage2 + 32 kernel sectors
db 2                       ; number of FATs
dw 224                     ; root directory entries
dw 2880                    ; total sectors (16-bit)
db 0xF0                    ; media descriptor
dw 9                       ; sectors per FAT
dw 18                      ; sectors per track
dw 2                       ; heads
dd 0                       ; hidden sectors
dd 0                       ; total sectors (32-bit)
db 0                       ; drive number (filled by BIOS at runtime is not required)
db 0                       ; reserved
db 0x29                    ; extended boot signature
dd 0x20261124              ; volume serial
db 'BINGOOS    '           ; volume label (11)
db 'FAT12   '              ; filesystem type (8)

boot_start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [0x7E00], dl        ; preserve BIOS boot drive for stage2

    ; Read stage2 from CHS 0/0/2 to 0000:8000.
    xor ax, ax
    mov es, ax
    mov bx, 0x8000
    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [0x7E00]
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

msg_disk_error db 'BingoOS: stage2 read error', 0

times 510-($-$$) db 0
dw 0xAA55
