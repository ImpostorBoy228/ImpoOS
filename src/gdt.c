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

// Расширяем GDT для поддержки ring3
// 0: NULL
// 1: Kernel Code (ring0)
// 2: Kernel Data (ring0)
// 3: User Code (ring3)
// 4: User Data (ring3)
// 5: TSS (для переключения задач, если понадобится)
struct gdt_entry gdt[6];
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
    gp.limit = (sizeof(struct gdt_entry) * 6) - 1;
    gp.base  = (uint64_t)&gdt;

    // Пустой дескриптор
    gdt_set_gate(0, 0, 0, 0, 0);
    // Дескриптор кода (Kernel Mode, 64-bit)
    // Access: 0x9A = 10011010b (Present, Ring 0, Code, Executable, Readable)
    // Granularity: 0x20 = Long Mode флаг
    gdt_set_gate(1, 0, 0xFFFFFFFF, 0x9A, 0x20);
    // Дескриптор данных (Kernel Mode)
    gdt_set_gate(2, 0, 0xFFFFFFFF, 0x92, 0x00);
    // Дескриптор кода (User Mode, ring3, 64-bit)
    // Access: 0xFA = 11111010b (Present, Ring 3, Code, Executable, Readable)
    gdt_set_gate(3, 0, 0xFFFFFFFF, 0xFA, 0x20);
    // Дескриптор данных (User Mode, ring3)
    // Access: 0xF2 = 11110010b (Present, Ring 3, Data, Writable)
    gdt_set_gate(4, 0, 0xFFFFFFFF, 0xF2, 0x00);
    // TSS (пока пустой, можно добавить позже)
    gdt_set_gate(5, 0, 0, 0, 0);

    __asm__ volatile("lgdt %0" : : "m"(gp));
}