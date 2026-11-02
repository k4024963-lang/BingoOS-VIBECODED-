include 'kernel\vga.inc'
include 'kernel\serial.inc'
include 'kernel\pic.inc'
include 'kernel\idt.inc'
include 'kernel\keyboard.inc'
include 'kernel\shell.inc'

kmain:
    cli
    cld

    call serial_init
    call vga_init

    ; Сначала настраиваем PIC и IDT, затем запускаем PIT.
    call pic_init
    call idt_init
    call pit_init

    ; Очистка буферов
    lea edi, [input_buffer]
    mov ecx, 128
    xor eax, eax
    rep stosb

    lea edi, [history_buffer]
    mov ecx, HISTORY_MAX * HISTORY_LEN
    xor eax, eax
    rep stosb

    mov esi, msg_banner
    call vga_puts

    sti

halt_loop:
    hlt
    jmp halt_loop

msg_banner db 'BingoOS Console v0.1', 13, 10, 0
