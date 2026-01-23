global start
extern long_mode_start

bits 32
section .text

start:
    cli
    mov esp, stack_top

    ; check long mode
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid
    bt edx, 29
    jc .lm_ok
.no_long_mode:
    hlt
.lm_ok:

    ; load PML4
    mov eax, pml4_table
    mov cr3, eax

    ; enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; enable long mode
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; load GDT
    lgdt [gdt64.pointer]

    ; enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ; jump to 64-bit
    jmp gdt64.code:long_mode_start

; =======================
; GDT
; =======================
section .rodata
gdt64:
    dq 0
.code: equ $ - gdt64
    dq 0x00AF9A000000FFFF  ; 64-bit code
    dq 0x00AF92000000FFFF  ; data
.pointer:
    dw $ - gdt64 - 1
    dq gdt64

; =======================
; Paging (identity map 1GiB)
; =======================
section .data
align 4096

pml4_table:
    dq pdpt_table + 0x03
    times 511 dq 0

pdpt_table:
    dq pd_table + 0x03
    times 511 dq 0


pd_table:
%assign i 0
%rep 512
    dq (i * 0x200000) | 0x83
%assign i i+1
%endrep

; =======================
; Stack
; =======================
section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
