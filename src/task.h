#ifndef TASK_H
#define TASK_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Структура контекста задачи (для переключения между задачами)
struct task_context {
    uint64_t rsp;      // Указатель стека
    uint64_t rip;      // Указатель инструкции
    uint64_t rflags;   // Флаги
    // Остальные регистры сохраняются на стеке при переключении
};

// Структура задачи
struct task {
    uint64_t id;                    // ID задачи
    struct task_context context;     // Контекст задачи
    uint64_t stack_base;            // Базовый адрес стека
    uint64_t stack_size;            // Размер стека
    bool is_running;                // Запущена ли задача
    uint64_t entry_point;           // Точка входа (адрес функции)
};

// Инициализация подсистемы задач
void task_init(void);

// Создание новой задачи в ring3
struct task* task_create(uint64_t entry_point, uint64_t stack_size);

// Переключение на задачу (переход в ring3)
void task_switch_to(struct task* task);

// Запуск задачи в ring3
void task_start(struct task* task);

#endif // TASK_H

