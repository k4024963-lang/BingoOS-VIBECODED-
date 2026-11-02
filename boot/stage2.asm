; AxionOS stage2 — GDT, A20, protected mode
format binary
org 0x8000

start:
    cli

    ; ES:BX = 0x1000:0x0000 = 0x10000 (куда грузим ядро)
    mov ax, 0x1000
    mov es, ax
    xor bx, bx
    mov ah, 0x02
    mov al, 32              ; 32 сектора = 16 КБ
    mov ch, 0
    mov cl, 3               ; начиная с сектора 3
    mov dh, 0
    mov dl, [0x7E00]        ; boot_drive из stage1
    int 0x13
    jc disk_error

    ; A20
    in al, 0x92
    or al, 2
    out 0x92, al

    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp 0x08:protected_mode

disk_error:
    cli
    hlt                     ; Просто вешаем систему
    jmp $

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

; --- Protected mode ---
use32
protected_mode:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000        ; стек

    jmp 0x10000             ; переход на entry ядра

; Добиваем до 512 байт
times 512-($-$$) db 0