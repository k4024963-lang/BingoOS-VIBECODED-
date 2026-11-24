; Primary ATA master, LBA28 PIO. Reads one 512-byte sector.
; Input: EAX=LBA, EDI=destination. Output: EAX=0 success, 1 error.
ATA_DATA equ 0x1F0
ATA_COUNT equ 0x1F2
ATA_LBA0 equ 0x1F3
ATA_LBA1 equ 0x1F4
ATA_LBA2 equ 0x1F5
ATA_DRIVE equ 0x1F6
ATA_STATUS equ 0x1F7
ATA_COMMAND equ 0x1F7

ata_read_sector:
    pushad
    mov [ata_lba_temp], eax
    mov [ata_dest_temp], edi

    mov eax, [ata_lba_temp]
    mov dx, ATA_DRIVE
    shr eax, 24
    and al, 0x0F
    or al, 0xE0
    out dx, al

    mov dx, ATA_COUNT
    mov al, 1
    out dx, al
    mov eax, [ata_lba_temp]
    mov dx, ATA_LBA0
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA1
    out dx, al
    shr eax, 8
    mov dx, ATA_LBA2
    out dx, al
    mov dx, ATA_COMMAND
    mov al, 0x20
    out dx, al

    call ata_wait_drq
    test eax, eax
    jnz .error
    mov dx, ATA_DATA
    mov edi, [ata_dest_temp]
    mov ecx, 256
    cld
    rep insw
    mov byte [ata_last_error], 0
    jmp .done
.error:
    mov byte [ata_last_error], 1
.done:
    popad
    movzx eax, byte [ata_last_error]
    ret

ata_wait_drq:
    mov ecx, 0x00100000
.wait:
    mov dx, ATA_STATUS
    in al, dx
    cmp al, 0xFF
    je .error
    test al, 0x80
    jnz .again
    test al, 0x01
    jnz .error
    test al, 0x08
    jnz .ok
.again:
    dec ecx
    jnz .wait
.error:
    mov eax, 1
    ret
.ok:
    xor eax, eax
    ret

ata_lba_temp dd 0
ata_dest_temp dd 0
ata_last_error db 0
