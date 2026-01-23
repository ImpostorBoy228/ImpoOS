#include "io.h"
#include "tty.h"
extern void terminal_putchar(char c);

// Простейшая таблица скан-кодов (Сет 1)
char kbd_us[128] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	'9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0, '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

void keyboard_handler_main() {
    uint8_t scancode = inb(0x60);

    // Если 7-й бит не установлен — это нажатие клавиши (а не отпускание)
    if (!(scancode & 0x80)) {
        if (kbd_us[scancode] != 0) {
            char c = kbd_us[scancode];
            
            // Записываем в TTY буфер
            tty_putchar_input(c);
            
            // Если включен echo, выводим на экран
            struct tty* t = tty_get_current();
            if (t->echo) {
                terminal_putchar(c);
            }
        }
    }

    // Отправляем сигнал конца прерывания контроллеру
    outb(0x20, 0x20);
}