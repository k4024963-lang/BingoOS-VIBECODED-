shell_execute:
    cmp dword [input_len], 0
    je .ret

    ; help
    mov esi, input_buffer
    mov edi, str_help
    call strcmp
    cmp eax, 1
    je .do_help

    ; type
    cmp byte [input_buffer], 't'
    jne .check_new

    cmp byte [input_buffer + 1], 'y'
    jne .check_new
    cmp byte [input_buffer + 2], 'p'
    jne .check_new
    cmp byte [input_buffer + 3], 'e'
    jne .check_new

    mov al, [input_buffer + 4]
    test al, al
    jz .do_type
    cmp al, ' '
    je .do_type
    cmp al, '('
    je .do_type
    jmp .check_new

.check_new:
    ; screencls
    mov esi, input_buffer
    mov edi, str_screencls
    call strcmp
    cmp eax, 1
    je .do_screencls

    ; bosver
    mov esi, input_buffer
    mov edi, str_bosver
    call strcmp
    cmp eax, 1
    je .do_bosver

    ; aboutbos
    mov esi, input_buffer
    mov edi, str_aboutbos
    call strcmp
    cmp eax, 1
    je .do_aboutbos

    ; /r и /t
    cmp byte [input_buffer], '/'
    jne .unknown

    cmp byte [input_buffer + 1], 'r'
    je .check_restart

    cmp byte [input_buffer + 1], 't'
    je .check_time

    cmp byte [input_buffer + 1], 's'
    je .check_shutdown

    jmp .unknown

.check_restart:
    mov al, [input_buffer + 2]
    test al, al
    jz .do_restart
    cmp al, ' '
    je .do_restart
    jmp .unknown

.check_time:
    mov al, [input_buffer + 2]
    test al, al
    jz .do_time
    cmp al, ' '
    je .do_time
    jmp .unknown

.check_shutdown:
    mov al, [input_buffer + 2]
    test al, al
    jz .do_shutdown
    jmp .unknown

.do_help:
    mov esi, msg_help
    call vga_puts
    jmp .ret

.do_type:
    lea esi, [input_buffer + 4]

.find_start:
    mov al, [esi]
    test al, al
    jz .ret

    cmp al, ' '
    je .next_char
    cmp al, '('
    je .next_char
    cmp al, '"'
    je .next_char

    jmp .find_end

.next_char:
    inc esi
    jmp .find_start

.find_end:
    mov edi, esi

.find_loop:
    mov al, [edi]
    test al, al
    jz .print_it

    cmp al, ')'
    je .cut_end
    cmp al, '"'
    je .cut_end

    inc edi
    jmp .find_loop

.cut_end:
    mov byte [edi], 0

.print_it:
    call vga_puts

    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc

    jmp .ret

.do_restart:
    mov al, 0xFE
    out 0x64, al
    jmp .ret

.do_time:
    call print_time
    jmp .ret

.do_screencls:
    call vga_clear
    jmp .ret

.do_bosver:
    mov esi, msg_bosver
    call vga_puts
    jmp .ret

.do_aboutbos:
    mov esi, msg_aboutbos
    call vga_puts
    jmp .ret

.do_shutdown:
    mov dx, 0x0604
    mov ax, 0x2000
    out dx, ax
    jmp $

.unknown:
    mov esi, msg_unknown
    call vga_puts
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc

.ret:
    ret

print_time:
    pushad

    ; Ждём завершения обновления RTC (UIP=0), затем читаем регистры.
.wait_uip:
    mov al, 0x0A
    out 0x70, al
    in al, 0x71
    test al, 0x80
    jnz .wait_uip

    mov al, 0x00
    out 0x70, al
    in al, 0x71
    mov [rtc_seconds], al

    mov al, 0x02
    out 0x70, al
    in al, 0x71
    mov [rtc_minutes], al

    mov al, 0x04
    out 0x70, al
    in al, 0x71
    mov [rtc_hours], al

    mov al, 0x0B
    out 0x70, al
    in al, 0x71
    mov [rtc_status_b], al

    ; В 12-часовом режиме старший бит часа означает PM.
    mov al, [rtc_hours]
    mov [rtc_pm], 0
    test al, 0x80
    jz .hour_pm_done
    and al, 0x7F
    mov [rtc_hours], al
    mov byte [rtc_pm], 1
.hour_pm_done:

    ; Если RTC работает в BCD-режиме (DM=0), переводим BCD в число.
    test byte [rtc_status_b], 0x04
    jnz .binary_mode

    mov al, [rtc_seconds]
    call bcd_to_binary
    mov [rtc_seconds], al

    mov al, [rtc_minutes]
    call bcd_to_binary
    mov [rtc_minutes], al

    mov al, [rtc_hours]
    call bcd_to_binary
    mov [rtc_hours], al

.binary_mode:
    ; 12-hour -> 24-hour. 12 AM = 00, 12 PM = 12.
    test byte [rtc_status_b], 0x02
    jnz .print_values
    cmp byte [rtc_pm], 0
    je .hour_am
    cmp byte [rtc_hours], 12
    je .print_values
    add byte [rtc_hours], 12
    jmp .print_values
.hour_am:
    cmp byte [rtc_hours], 12
    jne .print_values
    mov byte [rtc_hours], 0

.print_values:
    mov al, [rtc_hours]
    call print_two_digits
    mov al, ':'
    call vga_putc
    mov al, [rtc_minutes]
    call print_two_digits
    mov al, ':'
    call vga_putc
    mov al, [rtc_seconds]
    call print_two_digits
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc

    popad
    ret

; Вход: AL = BCD. Выход: AL = двоичное число.
bcd_to_binary:
    push ebx
    mov bl, al
    and al, 0x0F
    mov bh, al
    mov al, bl
    shr al, 4
    and al, 0x0F
    mov bl, 10
    mul bl
    add al, bh
    pop ebx
    ret

; Вход: AL = число 0..99. Выводит две десятичные цифры.
print_two_digits:
    push eax
    push ebx
    xor ah, ah
    mov bl, 10
    div bl
    add al, '0'
    mov [time_digit_tens], al
    mov al, ah
    add al, '0'
    mov [time_digit_ones], al
    mov al, [time_digit_tens]
    call vga_putc
    mov al, [time_digit_ones]
    call vga_putc
    pop ebx
    pop eax
    ret

time_digit_tens db 0
time_digit_ones db 0
rtc_seconds db 0
rtc_minutes db 0
rtc_hours db 0
rtc_status_b db 0
rtc_pm db 0

strcmp:
.loop:
    mov al, [esi]
    mov bl, [edi]
    cmp al, bl
    jne .no
    test al, al
    jz .yes
    inc esi
    inc edi
    jmp .loop
.yes:
    mov eax, 1
    ret
.no:
    xor eax, eax
    ret

strncmp_4:
    mov ecx, 4
.loop:
    mov al, [esi]
    cmp al, [edi]
    jne .no
    inc esi
    inc edi
    dec ecx
    jnz .loop
    mov eax, 0
    ret
.no:
    mov eax, 1
    ret

str_help db 'help', 0
str_type db 'type', 0
str_screencls db 'screencls', 0
str_bosver    db 'bosver', 0
str_aboutbos  db 'aboutbos', 0
str_shutdown  db '/s', 0
msg_bosver db 'BingoOS Pre-Alpha', 13, 10
           db 'Build 01-test', 13, 10
           db '32-bit Operating System', 13, 10, 0

msg_aboutbos db 'BingoOS - Hobby Operating System', 13, 10
             db 'Author: @dteamcorporation', 13, 10
             db 'Development: Vibe Coding, using ChatGPT', 13, 10
             db 'Assembler: FASM 1.73.35', 13, 10, 0
msg_help db 'Commands:', 13, 10
         db 'help - command list', 13, 10
         db 'type("text") - print text', 13, 10
         db '/r - reboot', 13, 10
         db '/t - show time', 13, 10
         db 'screencls - clear screen', 13, 10
         db 'bosver - version', 13, 10
         db 'aboutbos - about BingoOS', 13, 10
         db '/s - shutdown', 13, 10, 0
msg_unknown db 'Unknown command.', 13, 10, 0