; Буфер ввода
input_buffer rb 128
input_len dd 0
input_pos dd 0
shift_pressed db 0
ctrl_pressed db 0
caps_locked db 0        ; <--- НОВОЕ: Для Caps Lock
extended_key db 0

; История команд
HISTORY_MAX equ 16
HISTORY_LEN equ 64
history_buffer rb HISTORY_MAX * HISTORY_LEN
history_count dd 0
history_index dd 0
draft_buffer rb 128

prompt_offset dd 0

; Таблицы скан-кодов
scan_code_to_ascii:
    db 0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 8, 9
    db 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 13, 0
    db 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', 39, '`', 0
    db '\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0
    db '*', 0, ' '

scan_code_to_ascii_shift:
    db 0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 8, 9
    db 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 13, 0
    db 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0
    db '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0
    db '*', 0, ' '

isr_keyboard:
    pusha
    cld
    in al, 0x60
    
    cmp al, 0xE0
    je .set_extended

    cmp byte [extended_key], 1
    je .handle_extended

    ; --- Модификаторы ---
    cmp al, 0x2A
    je .shift_on
    cmp al, 0x36
    je .shift_on
    cmp al, 0xAA
    je .shift_off
    cmp al, 0xB6
    je .shift_off

    cmp al, 0x1D
    je .ctrl_on
    cmp al, 0x9D
    je .ctrl_off

    cmp al, 0x3A          ; Caps Lock
    je .caps_toggle

    test al, 0x80
    jnz .done

    ; Горячие клавиши
    cmp byte [ctrl_pressed], 1
    jne .normal_keys
    cmp al, 0x10
    je .hotkey_cancel
    cmp al, 0x1E
    je .hotkey_end
    cmp al, 0x26
    je .hotkey_clear

.normal_keys:
    cmp al, 0x39
    je .space
    cmp al, 0x1C
    je .enter
    cmp al, 0x0E
    je .backspace

    cmp al, 0x02
    jb .done
    cmp al, 0x38
    ja .done

    ; --- ВЫБОР СИМВОЛА ---
    movzx ebx, al
    
    ; Сначала выбираем обычный символ.
    mov al, [scan_code_to_ascii + ebx]

    ; Для букв Shift и Caps Lock работают как XOR.
    cmp al, 'a'
    jb .not_letter
    cmp al, 'z'
    ja .not_letter
    mov dl, [shift_pressed]
    xor dl, [caps_locked]
    test dl, dl
    jz .check_ascii
    sub al, 32
    jmp .check_ascii

.not_letter:
    ; Для цифр и знаков действует только Shift.
    cmp byte [shift_pressed], 1
    jne .check_ascii
    mov al, [scan_code_to_ascii_shift + ebx]
    jmp .check_ascii

.check_ascii:
    test al, al
    jz .done
    jmp .print_char

.space:
    mov al, ' '
    jmp .print_char

; --- ВСТАВКА СИМВОЛА ---
.print_char:
    mov ecx, [input_len]
    cmp ecx, 127
    jge .done
    
    mov edi, ecx
    mov esi, ecx
    dec esi
.shift_right_loop:
    cmp edi, [input_pos]
    jle .insert_now
    mov bl, [input_buffer + esi]
    mov [input_buffer + edi], bl
    dec edi
    dec esi
    jmp .shift_right_loop

.insert_now:
    mov ebx, [input_pos]
    mov [input_buffer + ebx], al
    inc dword [input_len]
    inc dword [input_pos]
    mov ebx, [input_len]
    mov byte [input_buffer + ebx], 0
    call redraw_line
    jmp .done

.backspace:
    cmp dword [input_pos], 0
    je .done
    dec dword [input_pos]
    dec dword [input_len]
    
    mov ecx, [input_pos]
    mov edi, ecx
    mov esi, ecx
    inc esi
.shift_left_loop:
    cmp edi, [input_len]
    jge .finish_shift
    mov bl, [input_buffer + esi]
    mov [input_buffer + edi], bl
    inc edi
    inc esi
    jmp .shift_left_loop

.finish_shift:
    mov ebx, [input_len]
    mov byte [input_buffer + ebx], 0
    call redraw_line
    jmp .done

; --- ENTER ---
.enter:
    cmp dword [input_len], 0
    je .skip_save
    
    mov eax, [history_count]
    mov ebx, HISTORY_MAX
    xor edx, edx
    div ebx
    imul edx, HISTORY_LEN
    
    lea edi, [history_buffer + edx]
    mov esi, input_buffer
    mov ecx, 64
    rep movsb
    inc dword [history_count]

.skip_save:
    mov eax, [history_count]
    mov [history_index], eax

    ; 1. Переводим строку через vga_putc
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc
    
    ; 2. Выполняем команду
    call shell_execute
    
    mov dword [input_len], 0
    mov dword [input_pos], 0
    
    ; 3. БЕРЕМ РЕАЛЬНУЮ ПОЗИЦИЮ ИЗ VGA (а не прибавляем 160!)
    movzx eax, byte [vga_row]
    imul eax, 160
    mov [prompt_offset], eax
    
    ; 4. Выводим приглашение
    mov esi, prompt
    call vga_puts
    jmp .done

.shift_on:
    mov byte [shift_pressed], 1
    jmp .done
.shift_off:
    mov byte [shift_pressed], 0
    jmp .done
.ctrl_on:
    mov byte [ctrl_pressed], 1
    jmp .done
.ctrl_off:
    mov byte [ctrl_pressed], 0
    jmp .done
.caps_toggle:
    xor byte [caps_locked], 1
    jmp .done

.set_extended:
    mov byte [extended_key], 1
    jmp .done

.handle_extended:
    mov byte [extended_key], 0
    test al, 0x80
    jnz .done

    cmp al, 0x4B
    je .arrow_left
    cmp al, 0x4D
    je .arrow_right
    cmp al, 0x48
    je .arrow_up
    cmp al, 0x50
    je .arrow_down
    jmp .done

.arrow_left:
    cmp dword [input_pos], 0
    je .done
    dec dword [input_pos]
    call redraw_line
    jmp .done

.arrow_right:
    mov eax, [input_pos]
    cmp eax, [input_len]
    jge .done
    inc dword [input_pos]
    call redraw_line
    jmp .done

.arrow_up:
    cmp dword [history_index], 0
    je .done
    mov eax, [history_count]
    cmp [history_index], eax
    jne .load_prev
    mov esi, input_buffer
    mov edi, draft_buffer
    mov ecx, 128
    rep movsb
.load_prev:
    dec dword [history_index]
    mov eax, [history_index]
    mov ebx, HISTORY_MAX
    xor edx, edx
    div ebx
    imul edx, HISTORY_LEN
    lea esi, [history_buffer + edx]
    mov edi, input_buffer
    mov ecx, HISTORY_LEN
    rep movsb
    mov edi, input_buffer
    add edi, HISTORY_LEN
    mov ecx, 64
    xor al, al
    rep stosb
    call strlen
    mov [input_len], eax
    mov [input_pos], eax
    call redraw_line
    jmp .done

.arrow_down:
    mov eax, [history_count]
    cmp [history_index], eax
    jge .done
    inc dword [history_index]
    mov eax, [history_count]
    cmp [history_index], eax
    jne .load_next
    mov esi, draft_buffer
    jmp .copy_next
.load_next:
    mov eax, [history_index]
    mov ebx, HISTORY_MAX
    xor edx, edx
    div ebx
    imul edx, HISTORY_LEN
    lea esi, [history_buffer + edx]
.copy_next:
    mov edi, input_buffer
    mov ecx, HISTORY_LEN
    rep movsb
    mov edi, input_buffer
    add edi, HISTORY_LEN
    mov ecx, 64
    xor al, al
    rep stosb
    call strlen
    mov [input_len], eax
    mov [input_pos], eax
    call redraw_line
    jmp .done

.hotkey_cancel:
    mov dword [input_len], 0
    mov dword [input_pos], 0
    call redraw_line
    jmp .done
.hotkey_end:
    mov eax, [input_len]
    mov [input_pos], eax
    call redraw_line
    jmp .done
.hotkey_clear:
    mov edi, 0xB8000
    mov ecx, 80 * 25
    mov ax, 0x0720
    rep stosw
    mov byte [vga_row], 0
    mov byte [vga_col], 0
    mov dword [prompt_offset], 0
    mov esi, prompt
    call vga_puts
    jmp .done

.done:
    mov al, 0x20
    out 0x20, al
    popa
    iretd

strlen:
    push esi
    xor eax, eax
.loop:
    cmp byte [esi + eax], 0
    je .done
    inc eax
    jmp .loop
.done:
    pop esi
    ret

redraw_line:
    pusha
    cld

    ; Проверяем prompt_offset.
    mov eax, [prompt_offset]
    shr eax, 1
    cmp eax, 80 * 25
    jb .valid_start

    xor eax, eax
    mov [prompt_offset], eax

.valid_start:
    mov ebp, eax                 ; начальная ячейка экрана

    ; Очищаем остаток экрана.
    mov edi, 0xB8000
    lea edi, [edi + ebp * 2]

    mov ecx, 80 * 25
    sub ecx, ebp
    mov ax, 0x0720
    rep stosw

    ; Выводим "> ".
    mov edi, 0xB8000
    lea edi, [edi + ebp * 2]

    mov ax, 0x073E
    stosw
    mov ax, 0x0720
    stosw

    ; Ограничиваем вывод доступным местом.
    mov esi, input_buffer
    mov ecx, [input_len]

    mov edx, 80 * 25
    sub edx, ebp
    cmp edx, 2
    jbe .update_cursor

    sub edx, 2
    cmp ecx, edx
    jbe .print_loop

    mov ecx, edx

.print_loop:
    test ecx, ecx
    jz .update_cursor

    lodsb
    mov ah, 0x07
    stosw
    dec ecx
    jmp .print_loop

.update_cursor:
    ; Позиция курсора.
    mov eax, ebp
    add eax, 2
    add eax, [input_pos]

    cmp eax, (80 * 25) - 1
    jbe .cursor_valid

    mov eax, (80 * 25) - 1

.cursor_valid:
    xor edx, edx
    mov ecx, 80
    div ecx

    mov [vga_row], al
    mov [vga_col], dl

    popa
    ret

prompt db '> ', 0