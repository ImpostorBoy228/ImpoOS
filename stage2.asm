[BITS 16]
[ORG 0x0600]  ; Теперь ORG соответствует фактическому адресу загрузки

start:
cli

; Включаем A20 line (простой метод через порт 0x92)
in al, 0x92
or al, 2
out 0x92, al

; Загружаем kernel.bin (предполагаем 1 сектор) в память по адресу 0x1000
mov ah, 0x02       ; BIOS read sectors
mov al, 1          ; читаем 1 сектор (если kernel больше, увеличьте al и скорректируйте)
mov ch, 0          ; cylinder 0
mov cl, 3          ; сектор 3 (kernel.bin)
mov dh, 0          ; head 0
mov dl, 0x80       ; первый HDD
mov bx, 0x1000     ; куда загружаем kernel.bin
int 0x13

; Подготавливаем GDT
lgdt [gdt_descriptor]

; Входим в protected mode
mov eax, cr0
or eax, 1
mov cr0, eax

; Far jump в 32-битный код
jmp 0x08:protected_mode

; GDT
gdt_null:
    dd 0
    dd 0

gdt_code:
    dw 0xFFFF    ; limit
    dw 0x0000    ; base
    db 0x00      ; base
    db 10011010b ; access
    db 11001111b ; granularity
    db 0x00      ; base high

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_null - 1
    dd gdt_null

[BITS 32]
protected_mode:
    ; Устанавливаем сегменты данных
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Прыгаем на kernel (в 0x1000)
    jmp 0x1000

; Заполняем до конца сектора 512 байт
times 512-($-start) db 0
