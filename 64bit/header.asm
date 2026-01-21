section .multiboot_header
header_start:
    ; Магическое число Multiboot 2
    dd 0xe85250d6
    ; Архитектура: 0 (i386 - 32-bit protected mode)
    dd 0
    ; Длина заголовка
    dd header_end - header_start
    ; Чексумма
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; Тег завершения
    dw 0
    dw 0
    dd 8
header_end: