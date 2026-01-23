section .multiboot_header
align 8

header_start:
    dd 0xe85250d6          ; magic
    dd 0                  ; architecture (i386)
    dd header_end - header_start
    dd 0x100000000 - (0xe85250d6 + 0 + (header_end - header_start))

    ; end tag
    dw 0
    dw 0
    dd 8
header_end:
