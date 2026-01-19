[BITS 16]
[ORG 0x7C00]

; BIOS: читаем stage2 (сектор 2)
mov ah, 0x02       ; BIOS read sectors
mov al, 1          ; читаем 1 сектор
mov ch, 0
mov cl, 2          ; сектор 2
mov dh, 0
mov dl, 0x80
mov bx, 0x0600     ; адрес загрузки stage2
int 0x13

jmp 0x0000:0x0600

; Заполняем до 510 байт
times 510-($-$$) db 0
dw 0xAA55
