shell_execute:
    cmp dword [input_len], 0
    je .ret

    mov esi, input_buffer
    mov edi, str_help
    call strcmp
    cmp eax, 1
    je .do_help

    cmp byte [input_buffer], 't'
    jne .check_fs
    cmp byte [input_buffer+1], 'y'
    jne .check_fs
    cmp byte [input_buffer+2], 'p'
    jne .check_fs
    cmp byte [input_buffer+3], 'e'
    jne .check_fs
    mov al, [input_buffer+4]
    test al, al
    jz .do_type
    cmp al, ' '
    je .do_type
    cmp al, '('
    je .do_type
    jmp .check_fs

.check_fs:
    mov esi, input_buffer
    mov edi, str_ls
    call strcmp
    cmp eax, 1
    je .do_ls

    cmp byte [input_buffer], 'c'
    jne .check_screencls
    cmp byte [input_buffer+1], 'a'
    jne .check_screencls
    cmp byte [input_buffer+2], 't'
    jne .check_screencls
    cmp byte [input_buffer+3], ' '
    je .do_cat

.check_screencls:
    mov esi, input_buffer
    mov edi, str_screencls
    call strcmp
    cmp eax, 1
    je .do_screencls
    mov esi, input_buffer
    mov edi, str_bosver
    call strcmp
    cmp eax, 1
    je .do_bosver
    mov esi, input_buffer
    mov edi, str_aboutbos
    call strcmp
    cmp eax, 1
    je .do_aboutbos

    cmp byte [input_buffer], '/'
    jne .unknown
    cmp byte [input_buffer+1], 'r'
    je .check_restart
    cmp byte [input_buffer+1], 't'
    je .check_time
    cmp byte [input_buffer+1], 's'
    je .check_shutdown
    jmp .unknown

.check_restart:
    mov al, [input_buffer+2]
    test al, al
    jz .do_restart
    cmp al, ' '
    je .do_restart
    jmp .unknown
.check_time:
    mov al, [input_buffer+2]
    test al, al
    jz .do_time
    cmp al, ' '
    je .do_time
    jmp .unknown
.check_shutdown:
    mov al, [input_buffer+2]
    test al, al
    jz .do_shutdown
    jmp .unknown

.do_help:
    mov esi, msg_help
    call vga_puts
    jmp .ret
.do_type:
    lea esi, [input_buffer+4]
.type_skip:
    mov al, [esi]
    test al, al
    jz .ret
    cmp al, ' '
    je .type_next
    cmp al, '('
    je .type_next
    cmp al, '"'
    je .type_next
    jmp .type_find_end
.type_next:
    inc esi
    jmp .type_skip
.type_find_end:
    mov edi, esi
.type_scan:
    mov al, [edi]
    test al, al
    jz .type_print
    cmp al, ')'
    je .type_cut
    cmp al, '"'
    je .type_cut
    inc edi
    jmp .type_scan
.type_cut:
    mov byte [edi], 0
.type_print:
    call vga_puts
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc
    jmp .ret

.do_ls:
    call fat12_ls
    jmp .ret
.do_cat:
    lea esi, [input_buffer+4]
    call fat12_cat
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
.wait_uip:
    mov al, 0x0A
    out 0x70, al
    in al, 0x71
    test al, 0x80
    jnz .wait_uip
    mov al, 0
    out 0x70, al
    in al, 0x71
    mov [rtc_seconds], al
    mov al, 2
    out 0x70, al
    in al, 0x71
    mov [rtc_minutes], al
    mov al, 4
    out 0x70, al
    in al, 0x71
    mov [rtc_hours], al
    mov al, 0x0B
    out 0x70, al
    in al, 0x71
    mov [rtc_status_b], al
    mov al, [rtc_hours]
    mov byte [rtc_pm], 0
    test al, 0x80
    jz .hour_flag_done
    and al, 0x7F
    mov [rtc_hours], al
    mov byte [rtc_pm], 1
.hour_flag_done:
    test byte [rtc_status_b], 4
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
    test byte [rtc_status_b], 2
    jnz .print_values
    cmp byte [rtc_pm], 0
    je .am
    cmp byte [rtc_hours], 12
    je .print_values
    add byte [rtc_hours], 12
    jmp .print_values
.am:
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

str_help db 'help',0
str_ls db 'ls',0
str_screencls db 'screencls',0
str_bosver db 'bosver',0
str_aboutbos db 'aboutbos',0
msg_bosver db 'BingoOS Pre-Alpha',13,10,'Build 01-1.1',13,10,'32-bit Operating System',13,10,0
msg_aboutbos db 'BingoOS - Hobby Operating System',13,10,'Assembler: FASM 1.73.35',13,10,0
msg_help db 'Commands:',13,10,'help - command list',13,10,'ls - list FAT12 root files',13,10,'cat NAME.EXT - display a file',13,10,'type("text") - print text',13,10,'/r - reboot',13,10,'/t - show time',13,10,'screencls - clear screen',13,10,'bosver - version',13,10,'aboutbos - about BingoOS',13,10,'/s - shutdown',13,10,0
msg_unknown db 'Unknown command.',13,10,0
time_digit_tens db 0
time_digit_ones db 0
rtc_seconds db 0
rtc_minutes db 0
rtc_hours db 0
rtc_status_b db 0
rtc_pm db 0
