; PIC (Programmable Interrupt Controller) + PIT
PIC1_CMD  equ 0x20
PIC1_DATA equ 0x21
PIC2_CMD  equ 0xA0
PIC2_DATA equ 0xA1

PIT_CMD   equ 0x43
PIT_CH0   equ 0x40
PIT_HZ    equ 20
PIT_DIVISOR equ 59659       ; 1193182 / 20 ~= 59659

pic_init:
    ; Remap PIC: IRQ0..7 -> INT 32..39, IRQ8..15 -> INT 40..47
    mov al, 0x11
    out PIC1_CMD, al
    out PIC2_CMD, al

    mov al, 0x20
    out PIC1_DATA, al
    mov al, 0x28
    out PIC2_DATA, al

    mov al, 0x04
    out PIC1_DATA, al
    mov al, 0x02
    out PIC2_DATA, al

    mov al, 0x01
    out PIC1_DATA, al
    out PIC2_DATA, al

    ; Разрешаем IRQ0 (PIT) и IRQ1 (клавиатура).
    ; Остальные IRQ Master и все IRQ Slave маскируем.
    mov al, 0xFC
    out PIC1_DATA, al
    mov al, 0xFF
    out PIC2_DATA, al
    ret

; Настройка PIT Channel 0 на 20 Гц = период около 50 мс.
; Вызывать после idt_init и до sti.
pit_init:
    mov al, 0x36             ; channel 0, lobyte/hibyte, mode 3
    out PIT_CMD, al

    mov ax, PIT_DIVISOR
    out PIT_CH0, al          ; младший байт
    mov al, ah
    out PIT_CH0, al          ; старший байт
    ret
