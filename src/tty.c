#include "tty.h"
#include <stddef.h>

// Глобальный TTY (пока один, можно расширить для множественных терминалов)
static struct tty main_tty;

void tty_init(void) {
    struct tty* t = &main_tty;
    
    t->input_head = 0;
    t->input_tail = 0;
    t->input_count = 0;
    
    t->output_head = 0;
    t->output_tail = 0;
    t->output_count = 0;
    
    t->echo = true;
    t->canonical = true;
    
    t->line_pos = 0;
    t->line_buffer[0] = '\0';
}

// Запись символа в буфер ввода (вызывается из обработчика клавиатуры)
void tty_putchar_input(char c) {
    struct tty* t = &main_tty;
    
    // Если буфер полон, игнорируем
    if (t->input_count >= TTY_BUFFER_SIZE - 1) {
        return;
    }
    
    // Обработка специальных символов в canonical режиме
    if (t->canonical) {
        if (c == '\n' || c == '\r') {
            // Конец строки - добавляем символ и завершаем строку
            if (t->line_pos < TTY_MAX_LINE_LENGTH - 1) {
                t->line_buffer[t->line_pos] = '\n';
                t->line_buffer[t->line_pos + 1] = '\0';
                
                // Копируем строку в input_buffer
                size_t i = 0;
                while (t->line_buffer[i] != '\0' && t->input_count < TTY_BUFFER_SIZE - 1) {
                    t->input_buffer[t->input_head] = t->line_buffer[i];
                    t->input_head = (t->input_head + 1) % TTY_BUFFER_SIZE;
                    t->input_count++;
                    i++;
                }
                
                t->line_pos = 0;
                t->line_buffer[0] = '\0';
            }
        } else if (c == '\b' || c == 127) { // Backspace
            if (t->line_pos > 0) {
                t->line_pos--;
                t->line_buffer[t->line_pos] = '\0';
            }
        } else if (c >= 32 && c < 127) { // Печатные символы
            if (t->line_pos < TTY_MAX_LINE_LENGTH - 1) {
                t->line_buffer[t->line_pos] = c;
                t->line_pos++;
                t->line_buffer[t->line_pos] = '\0';
            }
        }
    } else {
        // Raw режим - просто добавляем символ
        t->input_buffer[t->input_head] = c;
        t->input_head = (t->input_head + 1) % TTY_BUFFER_SIZE;
        t->input_count++;
    }
}

// Чтение из TTY (для пользовательских программ)
size_t tty_read(char* buffer, size_t count) {
    struct tty* t = &main_tty;
    size_t read_count = 0;
    
    // Читаем из буфера ввода
    while (read_count < count && t->input_count > 0) {
        buffer[read_count] = t->input_buffer[t->input_tail];
        t->input_tail = (t->input_tail + 1) % TTY_BUFFER_SIZE;
        t->input_count--;
        read_count++;
    }
    
    return read_count;
}

// Запись в TTY (для пользовательских программ)
size_t tty_write(const char* buffer, size_t count) {
    struct tty* t = &main_tty;
    size_t written = 0;
    
    // Выводим напрямую на экран (как в Linux tty)
    extern void terminal_putchar(char c);
    for (size_t i = 0; i < count; i++) {
        terminal_putchar(buffer[i]);
        written++;
    }
    
    return written;
}

struct tty* tty_get_current(void) {
    return &main_tty;
}

