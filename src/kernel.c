#include <stdint.h>
#include <stddef.h>

extern void gdt_install();
extern void idt_install();
extern void pic_remap();
extern void terminal_clear();
extern void kprint(const char* str);
extern void tty_init(void);
extern void syscall_init(void);

void kernel_main() {
    gdt_install();
    idt_install();
    pic_remap(); // Настраиваем контроллер прерываний
    
    // Инициализация TTY подсистемы
    tty_init();
    
    // Инициализация системных вызовов
    syscall_init();
    
    terminal_clear();
    kprint("ImpoOS 64-bit is loaded!\n");
    kprint("TTY subsystem initialized.\n");
    kprint("System calls enabled.\n");
    kprint("Keyboard interrupts enabled.\n");
    kprint("Try to type something:\n\n> ");

    // ВКЛЮЧАЕМ прерывания в процессоре
    __asm__ volatile ("sti");

    while(1) {
        __asm__ volatile ("hlt");
    }
}