; BingoOS stage2 — load kernel, enable A20, enter 32-bit protected mode
format binary
org 0x8000

start:
    cli

    ; Kernel occupies LBA 2..33 (CHS 0/0/3..18, then 0/1/1..16).
    ; Split BIOS reads at the track boundary; do not issue a request
    ; that crosses a 1.44 MB floppy track.
    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    ; First 16 sectors: cylinder 0, head 0, sectors 3..18.
    mov ah, 0x02
    mov al, 16
    mov ch, 0
    mov cl, 3
    mov dh, 0
    mov dl, [0x7E00]
    int 0x13
    jc disk_error

    ; Next 16 sectors: cylinder 0, head 1, sectors 1..16.
    mov ax, 0x1000
    mov es, ax
    mov bx, 0x2000
    mov ah, 0x02
    mov al, 16
    mov ch, 0
    mov cl, 1
    mov dh, 1
    mov dl, [0x7E00]
    int 0x13
    jc disk_error

    ; Enable A20 through the fast A20 gate.
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

msg_disk_error db 'BingoOS: kernel read error', 0

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
