; AxionOS stage1 — MBR
format binary
org 0x7C00

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    mov [0x7E00], dl        ; сохраняем boot_drive

    mov ah, 0x02
    mov al, 1
    mov ch, 0
    mov cl, 2
    mov dh, 0
    mov dl, [0x7E00]
    mov bx, 0x8000
    int 0x13
    jc disk_error

    jmp 0x0000:0x8000

disk_error:
    mov si, msg_disk_error
    call print16
    hlt
    jmp $

print16:
    lodsb
    or al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp print16
.done:
    ret

msg_disk_error db 'Disk error!', 0

times 510-($-$$) db 0
dw 0xAA55