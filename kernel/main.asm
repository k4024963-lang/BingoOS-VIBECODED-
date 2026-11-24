include 'kernel\\vga.inc'
include 'kernel\\serial.inc'
include 'kernel\\pic.inc'
include 'kernel\\idt.inc'
include 'kernel\\keyboard.inc'
include 'kernel\\shell.inc'
include 'kernel\\ata.inc'
include 'kernel\\fat12.inc'

kmain:
    cli
    cld
    call serial_init
    call vga_init

    mov esi, msg_banner
    call vga_puts

    ; QEMU run.bat attaches bingo.img as primary IDE master.
    call fat12_mount
    test eax, eax
    jnz .fat12_failed
    mov esi, msg_fat12_ready
    call vga_puts
    jmp .init_interrupts
.fat12_failed:
    mov esi, msg_fat12_failed
    call vga_puts

.init_interrupts:
    call pic_init
    call idt_init
    call pit_init

    lea edi, [input_buffer]
    mov ecx, 128
    xor eax, eax
    rep stosb
    lea edi, [history_buffer]
    mov ecx, HISTORY_MAX * HISTORY_LEN
    xor eax, eax
    rep stosb

    mov esi, prompt
    call vga_puts
    sti

halt_loop:
    hlt
    jmp halt_loop

msg_banner db 'BingoOS Console v1.1 - prealpha',13,10,0
msg_fat12_ready db 'FAT12: mounted (read-only)',13,10,0
msg_fat12_failed db 'FAT12: mount failed',13,10,0
