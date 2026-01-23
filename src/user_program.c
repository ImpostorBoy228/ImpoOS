// Пример пользовательской программы для тестирования ring3 терминала
// Эта программа будет запускаться в ring3 и использовать системные вызовы

#include <stdint.h>
#include <stddef.h>

// Номера системных вызовов (должны совпадать с syscall.h)
#define SYS_READ   0
#define SYS_WRITE  1
#define SYS_EXIT   2

// Вспомогательная функция для системного вызова
static inline uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a" (ret)
        : "a" (num), "D" (arg1), "S" (arg2), "d" (arg3)
        : "rcx", "r11", "memory"
    );
    return ret;
}

// Вспомогательная функция для вычисления длины строки
static size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// Пользовательская программа
void user_main(void) {
    const char* msg = "Hello from ring3! This is a user program.\n";
    const char* prompt = "Enter something: ";
    char buffer[256];
    
    // Выводим приветствие
    syscall(SYS_WRITE, 1, (uint64_t)msg, strlen(msg));
    
    // Выводим промпт
    syscall(SYS_WRITE, 1, (uint64_t)prompt, strlen(prompt));
    
    // Читаем ввод
    uint64_t read_count = syscall(SYS_READ, 0, (uint64_t)buffer, 255);
    
    // Выводим прочитанное
    const char* echo = "You typed: ";
    syscall(SYS_WRITE, 1, (uint64_t)echo, strlen(echo));
    syscall(SYS_WRITE, 1, (uint64_t)buffer, read_count);
    
    // Завершаем программу
    syscall(SYS_EXIT, 0, 0, 0);
    
    // Бесконечный цикл на случай, если exit не работает
    while (1) {
        __asm__ volatile ("hlt");
    }
}

