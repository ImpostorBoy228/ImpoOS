#include <stdint.h>

// Структура записи в GDT
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

// Структура, которую понимает процессор (GDTR)
struct gdt_ptr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gp;

// Функция для установки записи в GDT
void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;
    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;
    gdt[num].granularity |= gran & 0xF0;
    gdt[num].access      = access;
}

// Загрузка GDT (реализуем через inline assembly)
void gdt_install() {
    gp.limit = (sizeof(struct gdt_entry) * 3) - 1;
    gp.base  = (uint64_t)&gdt;

    // Пустой дескриптор
    gdt_set_gate(0, 0, 0, 0, 0);
    // Дескриптор кода (Kernel Mode, 64-bit)
    // Access: 0x9A = 10011010b (Present, Ring 0, Code, Executable, Readable)
    // Granularity: 0x20 = Long Mode флаг
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20);
    // Дескриптор данных
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0x00);

    __asm__ volatile("lgdt %0" : : "m"(gp));
}