; FAT12 read-only driver for the 1.44 MiB volume.
; First implementation supports root directory and 8.3 names.
fat12_mount:
    pushad
    mov edi, fat12_boot_sector
    xor eax, eax
    call ata_read_sector
    test eax, eax
    jnz .fail
    cmp word [fat12_boot_sector+11], 512
    jne .fail
    cmp byte [fat12_boot_sector+13], 1
    jne .fail
    cmp byte [fat12_boot_sector+16], 2
    jne .fail

    movzx eax, word [fat12_boot_sector+14]
    mov [fat12_reserved], eax
    movzx eax, word [fat12_boot_sector+22]
    mov [fat12_spf], eax
    movzx eax, word [fat12_boot_sector+17]
    mov [fat12_root_entries], eax

    mov eax, [fat12_reserved]
    mov [fat12_fat_lba], eax
    mov eax, [fat12_spf]
    shl eax, 1
    add eax, [fat12_reserved]
    mov [fat12_root_lba], eax

    mov eax, [fat12_root_entries]
    shl eax, 5
    add eax, 511
    shr eax, 9
    mov [fat12_root_sectors], eax
    add eax, [fat12_root_lba]
    mov [fat12_data_lba], eax

    mov esi, [fat12_fat_lba]
    mov ecx, [fat12_spf]
    mov edi, fat12_fat_buffer
.fat_loop:
    test ecx, ecx
    jz .root_start
    mov eax, esi
    call ata_read_sector
    test eax, eax
    jnz .fail
    inc esi
    add edi, 512
    dec ecx
    jmp .fat_loop

.root_start:
    mov esi, [fat12_root_lba]
    mov ecx, [fat12_root_sectors]
    mov edi, fat12_root_buffer
.root_loop:
    test ecx, ecx
    jz .ok
    mov eax, esi
    call ata_read_sector
    test eax, eax
    jnz .fail
    inc esi
    add edi, 512
    dec ecx
    jmp .root_loop
.ok:
    mov byte [fat12_is_mounted], 1
    popad
    xor eax, eax
    ret
.fail:
    mov byte [fat12_is_mounted], 0
    popad
    mov eax, 1
    ret

; EAX=cluster; returns EAX=FAT12 next cluster.
fat12_next_cluster:
    push ebx
    push edx
    mov edx, eax
    mov ebx, eax
    shr ebx, 1
    add ebx, edx
    movzx eax, word [fat12_fat_buffer+ebx]
    test edx, 1
    jz .even
    shr eax, 4
    jmp .done
.even:
    and eax, 0x0FFF
.done:
    and eax, 0x0FFF
    pop edx
    pop ebx
    ret

fat12_ls:
    cmp byte [fat12_is_mounted], 1
    jne fat12_not_mounted
    pushad
    mov esi, fat12_root_buffer
    mov eax, [fat12_root_entries]
    mov [fat12_ls_remaining], eax
.entry:
    cmp dword [fat12_ls_remaining], 0
    je .done
    cmp byte [esi], 0
    je .done
    cmp byte [esi], 0xE5
    je .next
    mov al, [esi+11]
    cmp al, 0x0F
    je .next
    test al, 0x08
    jnz .next
    xor ebx, ebx
.base:
    cmp ebx, 8
    jae .ext_start
    mov al, [esi+ebx]
    cmp al, ' '
    je .base_next
    push ebx
    call vga_putc
    pop ebx
.base_next:
    inc ebx
    jmp .base
.ext_start:
    cmp byte [esi+8], ' '
    je .newline
    mov al, '.'
    call vga_putc
    mov ebx, 8
.ext_loop:
    cmp ebx, 11
    jae .newline
    mov al, [esi+ebx]
    cmp al, ' '
    je .ext_next
    push ebx
    call vga_putc
    pop ebx
.ext_next:
    inc ebx
    jmp .ext_loop
.newline:
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc
.next:
    add esi, 32
    dec dword [fat12_ls_remaining]
    jmp .entry
.done:
    popad
    ret

; ESI points to an 8.3 name, for example README.TXT.
fat12_cat:
    cmp byte [fat12_is_mounted], 1
    jne fat12_not_mounted
    pushad
    mov edi, fat12_name_buffer
    mov ecx, 11
    mov al, ' '
    rep stosb
    mov edi, fat12_name_buffer
    xor ebx, ebx
.base_parse:
    lodsb
    test al, al
    jz .search_start
    cmp al, ' '
    je .search_start
    cmp al, '.'
    je .ext_start
    cmp ebx, 8
    jae .bad_name
    call fat12_uppercase
    mov [edi], al
    inc edi
    inc ebx
    jmp .base_parse
.ext_start:
    xor ebx, ebx
.ext_parse:
    lodsb
    test al, al
    jz .search_start
    cmp al, ' '
    je .search_start
    cmp ebx, 3
    jae .bad_name
    call fat12_uppercase
    mov [edi], al
    inc edi
    inc ebx
    jmp .ext_parse

.search_start:
    mov ebp, fat12_root_buffer
    mov edx, [fat12_root_entries]
.search:
    test edx, edx
    jz .not_found
    cmp byte [ebp], 0
    je .not_found
    cmp byte [ebp], 0xE5
    je .search_next
    mov al, [ebp+11]
    cmp al, 0x0F
    je .search_next
    test al, 0x08
    jnz .search_next
    push esi
    push edi
    push ecx
    mov esi, ebp
    mov edi, fat12_name_buffer
    mov ecx, 11
    repe cmpsb
    pop ecx
    pop edi
    pop esi
    je .found
.search_next:
    add ebp, 32
    dec edx
    jmp .search

.found:
    movzx eax, word [ebp+26]
    mov [fat12_current_cluster], eax
    mov eax, [ebp+28]
    mov [fat12_bytes_remaining], eax
    test eax, eax
    jz .finish
.cluster_loop:
    mov eax, [fat12_current_cluster]
    cmp eax, 2
    jb .read_error
    cmp eax, 4084
    ja .read_error
    sub eax, 2
    add eax, [fat12_data_lba]
    mov edi, fat12_sector_buffer
    call ata_read_sector
    test eax, eax
    jnz .read_error

    mov eax, [fat12_bytes_remaining]
    cmp eax, 512
    jbe .last_count
    mov dword [fat12_print_remaining], 512
    jmp .print_start
.last_count:
    mov [fat12_print_remaining], eax
.print_start:
    mov esi, fat12_sector_buffer
.print_loop:
    cmp dword [fat12_print_remaining], 0
    je .sector_done
    lodsb
    call vga_putc
    dec dword [fat12_print_remaining]
    dec dword [fat12_bytes_remaining]
    jmp .print_loop
.sector_done:
    cmp dword [fat12_bytes_remaining], 0
    je .finish
    mov eax, [fat12_current_cluster]
    call fat12_next_cluster
    cmp eax, 0x0FF8
    jae .read_error
    mov [fat12_current_cluster], eax
    jmp .cluster_loop

.bad_name:
    mov esi, msg_fat12_bad_name
    call vga_puts
    jmp .finish
.not_found:
    mov esi, msg_fat12_not_found
    call vga_puts
    jmp .finish
.read_error:
    mov esi, msg_fat12_read_error
    call vga_puts
.finish:
    mov al, 13
    call vga_putc
    mov al, 10
    call vga_putc
    popad
    ret

fat12_uppercase:
    cmp al, 'a'
    jb .done
    cmp al, 'z'
    ja .done
    sub al, 32
.done:
    ret

fat12_not_mounted:
    mov esi, msg_fat12_not_mounted
    call vga_puts
    ret

fat12_boot_sector rb 512
fat12_fat_buffer rb 9*512
fat12_root_buffer rb 14*512
fat12_sector_buffer rb 512
fat12_name_buffer rb 11
fat12_reserved dd 0
fat12_spf dd 0
fat12_root_entries dd 0
fat12_fat_lba dd 0
fat12_root_lba dd 0
fat12_root_sectors dd 0
fat12_data_lba dd 0
fat12_current_cluster dd 0
fat12_bytes_remaining dd 0
fat12_print_remaining dd 0
fat12_ls_remaining dd 0
fat12_is_mounted db 0
msg_fat12_not_mounted db 'FAT12: volume not mounted',13,10,0
msg_fat12_bad_name db 'Usage: cat NAME.EXT',13,10,0
msg_fat12_not_found db 'FAT12: file not found',13,10,0
msg_fat12_read_error db 'FAT12: disk read error or invalid cluster chain',13,10,0
