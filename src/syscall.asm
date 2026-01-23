[bits 64]

extern syscall_handler
global syscall_handler_asm
global syscall_return_value

section .data
syscall_return_value: dq 0

section .text
syscall_handler_asm:
    ; При syscall процессор автоматически:
    ; - Сохраняет RIP в RCX
    ; - Сохраняет RFLAGS в R11
    ; - Переключается на ring0
    
    ; Сохраняем регистры, которые могут быть изменены
    ; (согласно calling convention x86-64, сохраняем callee-saved)
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15
    
    ; Сохраняем RCX и R11 (они содержат RIP и RFLAGS)
    push rcx
    push r11
    
    ; Системный вызов использует:
    ; RAX = номер системного вызова
    ; RDI = arg1
    ; RSI = arg2
    ; RDX = arg3
    
    ; Подготавливаем аргументы для вызова C функции
    ; syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3)
    mov rcx, rdx        ; arg3 -> rcx (4-й аргумент в x86-64)
    mov rdx, rsi        ; arg2 -> rdx (3-й аргумент)
    mov rsi, rdi        ; arg1 -> rsi (2-й аргумент)
    mov rdi, rax        ; syscall_num -> rdi (1-й аргумент)
    
    ; Вызываем обработчик
    call syscall_handler
    
    ; Результат должен быть в syscall_return_value
    ; Загружаем его в RAX (возвращаемое значение)
    ; Используем RIP-relative адресацию для 64-bit режима
    mov rax, [rel syscall_return_value]
    
    ; Восстанавливаем RCX и R11
    pop r11
    pop rcx
    
    ; Восстанавливаем остальные регистры
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    
    ; Возвращаемся в пользовательский режим
    ; sysret использует:
    ; - RCX для RIP (восстановлен выше)
    ; - R11 для RFLAGS (восстановлен выше)
    sysret

