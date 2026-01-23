#ifndef TTY_H
#define TTY_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define TTY_BUFFER_SIZE 4096
#define TTY_MAX_LINE_LENGTH 256

// Структура TTY устройства
struct tty {
    // Буфер ввода (для чтения из ring3)
    char input_buffer[TTY_BUFFER_SIZE];
    size_t input_head;  // Позиция записи
    size_t input_tail;  // Позиция чтения
    size_t input_count; // Количество символов в буфере
    
    // Буфер вывода (для записи в ring3)
    char output_buffer[TTY_BUFFER_SIZE];
    size_t output_head;
    size_t output_tail;
    size_t output_count;
    
    // Флаги состояния
    bool echo;  // Эхо ввода (отображать нажатые клавиши)
    bool canonical; // Канонический режим (обработка строк)
    
    // Текущая строка (для canonical режима)
    char line_buffer[TTY_MAX_LINE_LENGTH];
    size_t line_pos;
};

// Инициализация TTY
void tty_init(void);

// Запись символа в TTY (из обработчика клавиатуры)
void tty_putchar_input(char c);

// Чтение из TTY (для ring3 через syscall)
size_t tty_read(char* buffer, size_t count);

// Запись в TTY (для ring3 через syscall)
size_t tty_write(const char* buffer, size_t count);

// Получить текущий TTY
struct tty* tty_get_current(void);

#endif // TTY_H

