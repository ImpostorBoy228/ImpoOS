global start
extern long_mode_start

section .text
bits 32
start:
    mov esp, stack_top

    ; 1. Проверка, поддерживает ли CPU режим Long Mode (упрощено)
    call check_cpuid
    call check_long_mode

    ; 2. Настройка страничной адресации (Paging)
    call set_up_page_tables
    call enable_paging

    ; 3. Загрузка 64-битной GDT
    lgdt [gdt64.pointer]

    ; 4. Прыжок в 64-битный код
    jmp gdt64.code_segment:long_mode_start

; --- Подпрограммы ---
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid
    ret
.no_cpuid:
    hlt ; CPUID не поддерживается, стоп.

check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29
    jz .no_long_mode
    ret
.no_long_mode:
    hlt ; Long mode не поддерживается.

set_up_page_tables:
    ; P4 указывает на P3
    mov eax, p3_table
    or eax, 0b11 ; present + writable
    mov [p4_table], eax

    ; P3 указывает на P2
    mov eax, p2_table
    or eax, 0b11
    mov [p3_table], eax

    ; P2 мапит первые 2MB (Huge Page)
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000 ; 2MiB
    mul ecx
    or eax, 0b10000011 ; present + writable + huge
    mov [p2_table + ecx * 8], eax
    inc ecx
    cmp ecx, 512
    jne .map_p2_table
    ret

enable_paging:
    ; Загружаем P4 в регистр CR3
    mov eax, p4_table
    mov cr3, eax

    ; Включаем PAE flag в CR4
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Включаем Long Mode в EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Включаем Paging в CR0
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    ret

; --- Данные (GDT) ---
section .rodata
gdt64:
    dq 0 ; Нулевой дескриптор
.code_segment: equ $ - gdt64
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53) ; Код
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

section .bss
align 4096
p4_table:
    resb 4096
p3_table:
    resb 4096
p2_table:
    resb 4096
stack_bottom:
    resb 4096 * 4
stack_top: