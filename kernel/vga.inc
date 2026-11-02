; VGA text mode 80x25
VGA_MEM    equ 0xB8000
VGA_WIDTH  equ 80
VGA_HEIGHT equ 25
VGA_CELLS  equ VGA_WIDTH * VGA_HEIGHT

vga_row  db 0
vga_col  db 0
vga_attr db 0x07

; Последняя позиция, отправленная в VGA CRTC.
; FFFFFFFF заставляет выполнить первую синхронизацию.
vga_cursor_last_pos dd 0xFFFFFFFF

vga_init:
    cld
    mov byte [vga_row], 0
    mov byte [vga_col], 0
    mov dword [vga_cursor_last_pos], 0xFFFFFFFF

    mov edi, VGA_MEM
    mov ecx, VGA_CELLS
    mov ax, 0x0720
    rep stosw

    ; Форма аппаратного курсора
    mov dx, 0x3D4
    mov al, 0x0A
    out dx, al
    mov dx, 0x3D5
    mov al, 14
    out dx, al

    mov dx, 0x3D4
    mov al, 0x0B
    out dx, al
    mov dx, 0x3D5
    mov al, 15
    out dx, al

    ; Начальная позиция; далее курсор обновляет PIT раз в 50 мс.
    call vga_cursor_sync
    ret

; Полная очистка экрана и сброс позиции курсора.
vga_clear:
    pusha
    cld

    mov edi, VGA_MEM
    mov ecx, VGA_CELLS
    mov ax, 0x0720
    rep stosw

    mov byte [vga_row], 0
    mov byte [vga_col], 0

    ; Принудительное обновление аппаратного курсора.
    mov dword [vga_cursor_last_pos], 0xFFFFFFFF
    call vga_cursor_sync

    popa
    ret

; Вычисляет позицию по vga_row/vga_col и перемещает курсор,
; только если позиция действительно изменилась.
vga_cursor_sync:
    pusha

    movzx eax, byte [vga_row]
    imul eax, VGA_WIDTH
    movzx ebx, byte [vga_col]
    add eax, ebx

    ; Защита от выхода за пределы текстового экрана.
    cmp eax, VGA_CELLS
    jb .position_valid
    mov eax, VGA_CELLS - 1

.position_valid:
    cmp eax, [vga_cursor_last_pos]
    je .done

    mov [vga_cursor_last_pos], eax
    mov ebx, eax

    ; Старший байт позиции
    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x3D5
    mov al, bh
    out dx, al

    ; Младший байт позиции
    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x3D5
    mov al, bl
    out dx, al

.done:
    popa
    ret

; Совместимость со старым именем функции.
vga_update_cursor:
    jmp vga_cursor_sync

; AL = символ
vga_putc:
    cmp al, 13
    je near vga_putc_cr
    cmp al, 10
    je near vga_putc_lf
    cmp al, 8
    je near vga_putc_bs

    movzx ebx, byte [vga_row]
    imul ebx, VGA_WIDTH
    movzx ecx, byte [vga_col]
    add ebx, ecx
    cmp ebx, VGA_CELLS
    jae vga_putc_done

    shl ebx, 1
    add ebx, VGA_MEM
    mov ah, [vga_attr]
    mov [ebx], ax

    inc byte [vga_col]
    cmp byte [vga_col], VGA_WIDTH
    jb near vga_putc_done
    mov byte [vga_col], 0
    inc byte [vga_row]
    jmp vga_putc_scroll

vga_putc_cr:
    mov byte [vga_col], 0
    jmp vga_putc_done

vga_putc_lf:
    inc byte [vga_row]
    jmp vga_putc_scroll

vga_putc_bs:
    cmp byte [vga_col], 0
    je near vga_putc_done
    dec byte [vga_col]
    jmp vga_putc_done

vga_putc_scroll:
    cmp byte [vga_row], VGA_HEIGHT
    jb near vga_putc_done
    call vga_scroll

vga_putc_done:
    ; Аппаратный курсор синхронизируется таймером каждые 50 мс.
    ret

vga_scroll:
    pusha
    cld

    mov edi, VGA_MEM
    mov esi, VGA_MEM + VGA_WIDTH * 2
    mov ecx, (VGA_HEIGHT - 1) * VGA_WIDTH
    rep movsw

    mov edi, VGA_MEM + (VGA_HEIGHT - 1) * VGA_WIDTH * 2
    mov ecx, VGA_WIDTH
    mov ax, 0x0720
    rep stosw

    dec byte [vga_row]

    popa
    ret

; AX = линейная позиция курсора (0..1999)
vga_set_cursor_pos:
    pusha
    movzx eax, ax
    cmp eax, VGA_CELLS
    jb .valid
    mov eax, VGA_CELLS - 1
.valid:
    mov [vga_cursor_last_pos], eax
    mov bx, ax

    mov dx, 0x3D4
    mov al, 0x0E
    out dx, al
    mov dx, 0x3D5
    mov al, bh
    out dx, al

    mov dx, 0x3D4
    mov al, 0x0F
    out dx, al
    mov dx, 0x3D5
    mov al, bl
    out dx, al

    popa
    ret

; ESI = ASCIIZ
vga_puts:
    cld
.next:
    lodsb
    test al, al
    jz .done
    call vga_putc
    jmp .next
.done:
    ret
