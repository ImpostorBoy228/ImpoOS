#!/bin/bash
set -e

WORKDIR=$(pwd)
echo "=== ImpoOS Build Start ==="

# --- 1) stage1.asm (MBR) ---
cat > stage1.asm <<'EOF'
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
EOF
echo "stage1.asm created"

# --- 2) stage2.asm (загрузка kernel с входом в protected mode) ---
cat > stage2.asm <<'EOF'
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
EOF
echo "stage2.asm created"

# --- 3) kernel.c ---
cat > kernel.c <<'EOF'
// kernel.c - freestanding 32-bit
void _start() {
    volatile unsigned short *video = (unsigned short*)0xB8000;
    
    // Очистка экрана: 80×25 = 2000 символов, белый на чёрном (0x0F00)
    for (int i = 0; i < 2000; i++) {
        video[i] = 0x0F00 | ' ';   // пробел с атрибутом белый на чёрном
    }
    
    // Теперь пишем "Hi"
    video[0] = 0x0F00 | 'H';
    video[1] = 0x0F00 | 'i';
    
    while(1) {}
}
EOF

echo "kernel.c created"

# Компиляция stage1
nasm -f bin stage1.asm -o stage1.bin

# Компиляция stage2
nasm -f bin stage2.asm -o stage2.bin

# Компиляция kernel
i686-elf-gcc -ffreestanding -m32 -c kernel.c -o kernel.o
i686-elf-ld -Ttext 0x1000 -m elf_i386 -o kernel.elf kernel.o
i686-elf-objcopy -O binary kernel.elf kernel.bin

# Выравниваем файлы до 512 байт (без перезаписи самих себя)
dd if=/dev/zero of=stage2_padded.bin bs=512 count=1 status=none
dd if=stage2.bin of=stage2_padded.bin conv=notrunc status=none
mv stage2_padded.bin stage2.bin

dd if=/dev/zero of=kernel_padded.bin bs=512 count=1 status=none
dd if=kernel.bin of=kernel_padded.bin conv=notrunc status=none
mv kernel_padded.bin kernel.bin

# Создание образа os.img
cat stage1.bin stage2.bin kernel.bin > os.img

ls -lh stage1.bin stage2.bin kernel.bin os.img
echo "=== Build complete ==="