#include <stdint.h>
#include <stddef.h>

// Объявляем функции из других файлов, чтобы компилятор не ругался
extern void gdt_install();
extern void idt_install();
extern void kprint(const char* str);
extern void terminal_clear();

void kernel_main() {
    // 1. Инициализация критических таблиц
    gdt_install();
    idt_install();
    
    // 2. Инициализация экрана (используем функции терминала)
    terminal_clear();
    
    kprint("ImpoOS (GRUB edition) - 64 bit\n");
    kprint("----------------------------\n");
    kprint("GDT installed: OK (Ring 0 setup)\n");
    kprint("IDT installed: OK (Exceptions ready)\n");
    
    // 3. Тест прерываний (раскомментируй для проверки)
    // Внимание: если IDT настроена неверно, QEMU уйдет в бесконечный ребут (Triple Fault)
    // __asm__ volatile ("int $0"); 

    kprint("\nSystem is running in Ring 0.\n");
    kprint("Waiting for interrupts...\n");

    while(1) {
        __asm__ volatile ("hlt"); // Экономим ресурсы процессора
    }
}