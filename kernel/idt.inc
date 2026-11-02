IDT_SIZE equ 256

idt:
    rb IDT_SIZE * 8

idt_descriptor:
    dw IDT_SIZE * 8 - 1
    dd idt

set_idt_gate:
    push edi
    lea edi, [idt + eax * 8]
    mov word [edi], bx
    mov word [edi + 2], 0x08
    mov byte [edi + 4], 0
    mov byte [edi + 5], 0x8E
    shr ebx, 16
    mov word [edi + 6], bx
    pop edi
    ret

idt_init:
    cld
    mov ecx, 0
.loop:
    mov eax, ecx
    mov ebx, isr_default
    call set_idt_gate
    inc ecx
    cmp ecx, IDT_SIZE
    jb .loop

    mov eax, 0
    mov ebx, isr_exception_0
    call set_idt_gate

    ; PIC remapped: IRQ0 -> 32, IRQ1 -> 33
    mov eax, 32
    mov ebx, isr_timer
    call set_idt_gate

    mov eax, 33
    mov ebx, isr_keyboard
    call set_idt_gate

    lidt [idt_descriptor]
    ret

isr_exception_0:
    cli
    mov esi, msg_exception_0
    call vga_puts
.hang:
    hlt
    jmp .hang

; IRQ0: PIT 20 Hz (один вызов каждые ~50 мс).
isr_timer:
    pushad
    cld
    inc dword [timer_ticks]
    call vga_cursor_sync

    ; EOI для Master PIC
    mov al, 0x20
    out 0x20, al

    popad
    iretd

isr_default:
    cli
.hang:
    hlt
    jmp .hang

timer_ticks dd 0
msg_exception_0 db 13, 10, 'Exception: Divide by zero!', 13, 10, 0
