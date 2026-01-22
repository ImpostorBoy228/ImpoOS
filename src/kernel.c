#include <stdint.h>
#include <stddef.h>

extern void gdt_install();
extern void idt_install();
extern void pic_remap();
extern void terminal_clear();
extern void kprint(const char* str);

void kernel_main() {
    gdt_install();
    idt_install();
    pic_remap(); // Настраиваем контроллер прерываний
    
    terminal_clear();
    kprint("ImpoOS 64-bit is loaded!\n");
    kprint("Keyboard interrupts enabled.\n");
    kprint("Try to type something:\n\n> ");

    // ВКЛЮЧАЕМ прерывания в процессоре
    __asm__ volatile ("sti");

    while(1) {
        __asm__ volatile ("hlt");
    }
}