#include "syscall.h"
#include "tty.h"
#include <stdint.h>
#include <stddef.h>

extern void syscall_handler_asm(void);
extern uint64_t syscall_return_value;

// Инициализация системных вызовов
void syscall_init(void) {
    // Настройка MSR для syscall/sysret
    // IA32_STAR (0xC0000081): устанавливает селекторы для syscall
    // - STAR[47:32] = селектор кода ядра (0x08 = kernel code)
    // - STAR[63:48] = селектор кода пользователя (0x18 = user code, 0x18 = 3*8)
    uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x18 << 48);
    __asm__ volatile("wrmsr" : : "c"(0xC0000081), "a"((uint32_t)star), "d"((uint32_t)(star >> 32)));
    
    // IA32_LSTAR (0xC0000082): адрес обработчика syscall
    uint64_t handler_addr = (uint64_t)syscall_handler_asm;
    __asm__ volatile("wrmsr" : : "c"(0xC0000082), "a"((uint32_t)handler_addr), "d"((uint32_t)(handler_addr >> 32)));
    
    // IA32_FMASK (0xC0000084): маска флагов, которые очищаются при syscall
    // Очищаем флаг прерываний (IF) и направление (DF)
    uint64_t fmask = (1 << 9) | (1 << 10); // IF и DF
    __asm__ volatile("wrmsr" : : "c"(0xC0000084), "a"((uint32_t)fmask), "d"((uint32_t)(fmask >> 32)));
    
    // IA32_EFER (0xC0000080): включаем бит SCE (SYSCALL Enable)
    uint32_t efer_low, efer_high;
    __asm__ volatile("rdmsr" : "=a"(efer_low), "=d"(efer_high) : "c"(0xC0000080));
    uint64_t efer = ((uint64_t)efer_high << 32) | efer_low;
    efer |= (1 << 0); // SCE bit
    __asm__ volatile("wrmsr" : : "c"(0xC0000080), "a"((uint32_t)efer), "d"((uint32_t)(efer >> 32)));
}

// Обработчик системного вызова
void syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    // Результат системного вызова будет в rax
    // Пока просто заглушка, реализуем ниже
    (void)arg1;
    (void)arg2;
    (void)arg3;
    
    switch (syscall_num) {
        case SYS_READ:
            // arg1 = fd (игнорируем, используем TTY)
            // arg2 = buffer
            // arg3 = count
            // return = количество прочитанных байт
            {
                char* buffer = (char*)arg2;
                size_t count = (size_t)arg3;
                size_t read = tty_read(buffer, count);
                // Результат должен быть в rax, но мы вернем его через специальный механизм
                // Пока используем глобальную переменную
                syscall_return_value = read;
            }
            break;
            
        case SYS_WRITE:
            // arg1 = fd (игнорируем, используем TTY)
            // arg2 = buffer
            // arg3 = count
            // return = количество записанных байт
            {
                const char* buffer = (const char*)arg2;
                size_t count = (size_t)arg3;
                size_t written = tty_write(buffer, count);
                syscall_return_value = written;
            }
            break;
            
        case SYS_EXIT:
            // arg1 = exit code
            // Завершение процесса (пока просто заглушка)
            {
                // TODO: реализовать завершение процесса
                syscall_return_value = 0;
            }
            break;
            
        default:
            // Неизвестный системный вызов
            {
                syscall_return_value = (uint64_t)-1;
            }
            break;
    }
}

