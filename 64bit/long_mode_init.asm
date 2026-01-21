global long_mode_start
extern kernel_main

section .text
bits 64
long_mode_start:
    ; Обнуляем сегментные регистры (в 64 битах они не нужны, но для порядка)
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Вызываем наше ядро на C
    call kernel_main
    hlt