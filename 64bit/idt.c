#include <stdint.h>

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;        // Сегмент кода (наш 0x08 из GDT)
    uint8_t  ist;        // Interrupt Stack Table (пока 0)
    uint8_t  flags;      // Тип и атрибуты
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idtp;

void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_mid  = (base >> 16) & 0xFFFF;
    idt[num].base_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].sel       = sel;
    idt[num].ist       = 0;
    idt[num].flags     = flags; // 0x8E - прерывание включено, Ring 0
    idt[num].reserved  = 0;
}

// Заглушка для теста (обработчик ошибки)
void exception_handler() {
    // В будущем тут будет вывод сообщения об ошибке
    for(;;); 
}

void idt_install() {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint64_t)&idt;

    // Заполняем IDT нулями
    for(int i = 0; i < 256; i++) idt_set_gate(i, 0, 0, 0);

    // Пример: ставим обработчик на Division Error (0)
    idt_set_gate(0, (uint64_t)exception_handler, 0x08, 0x8E);

    __asm__ volatile("lidt %0" : : "m"(idtp));
}