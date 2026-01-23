[bits 64]

global task_enter_ring3

section .text
task_enter_ring3:
    ; Переход в ring3
    ; rdi = RIP (точка входа)
    ; rsi = RSP (указатель стека)
    
    ; Настраиваем селекторы для ring3
    ; 0x23 = 0x18 | 3 = user data segment (ring3)
    ; 0x1B = 0x18 | 3 = user code segment (ring3)
    
    ; Сохраняем текущий стек
    mov rcx, rsp
    
    ; Переключаемся на пользовательский стек
    mov rsp, rsi
    
    ; Подготавливаем стек для iretq
    ; Формат для iretq в 64-bit:
    ; [RSP+0x00] = RIP
    ; [RSP+0x08] = CS (code segment)
    ; [RSP+0x10] = RFLAGS
    ; [RSP+0x18] = RSP (stack pointer)
    ; [RSP+0x20] = SS (stack segment)
    
    push 0x23        ; SS (user data segment, ring3)
    push rsi         ; RSP (user stack)
    push 0x202       ; RFLAGS (с установленным IF)
    push 0x1B        ; CS (user code segment, ring3)
    push rdi         ; RIP (точка входа)
    
    ; Возвращаемся в пользовательский режим
    iretq

