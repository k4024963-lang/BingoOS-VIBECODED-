; COM1, 115200 8N1
SERIAL_PORT equ 0x3F8

serial_init:
    push eax
    push edx
    mov dx, SERIAL_PORT + 1
    mov al, 0x00
    out dx, al
    mov dx, SERIAL_PORT + 3
    mov al, 0x80
    out dx, al
    mov dx, SERIAL_PORT + 0
    mov al, 0x01
    out dx, al
    mov dx, SERIAL_PORT + 1
    mov al, 0x00
    out dx, al
    mov dx, SERIAL_PORT + 3
    mov al, 0x03
    out dx, al
    mov dx, SERIAL_PORT + 2
    mov al, 0xC7
    out dx, al
    pop edx
    pop eax
    ret

; AL = символ
serial_putc:
    push eax
    push edx
    mov ah, al
serial_putc_wait:
    mov dx, SERIAL_PORT + 5
    in al, dx
    test al, 0x20
    jz near serial_putc_wait
    mov dx, SERIAL_PORT
    mov al, ah
    out dx, al
    pop edx
    pop eax
    ret

; ESI = ASCIIZ
serial_puts:
    lodsb
    or al, al
    jz near serial_puts_done
    call serial_putc
    jmp serial_puts
serial_puts_done:
    ret