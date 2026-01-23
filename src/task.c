#include "task.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_TASKS 16
#define DEFAULT_STACK_SIZE 0x10000  // 64 KB

static struct task tasks[MAX_TASKS];
static uint64_t next_task_id = 1;
static struct task* current_task = NULL;

void task_init(void) {
    // Инициализация массива задач
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].id = 0;
        tasks[i].is_running = false;
        tasks[i].stack_base = 0;
        tasks[i].stack_size = 0;
        tasks[i].entry_point = 0;
    }
}

struct task* task_create(uint64_t entry_point, uint64_t stack_size) {
    // Находим свободный слот
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].id == 0) {
            struct task* t = &tasks[i];
            t->id = next_task_id++;
            t->entry_point = entry_point;
            t->stack_size = stack_size ? stack_size : DEFAULT_STACK_SIZE;
            
            // Выделяем стек (пока просто статический массив)
            // В реальной ОС нужно использовать аллокатор памяти
            static uint8_t task_stacks[MAX_TASKS][0x10000];
            t->stack_base = (uint64_t)task_stacks[i];
            
            // Настраиваем контекст
            // Стек растет вниз, поэтому начинаем с верхушки
            t->context.rsp = t->stack_base + t->stack_size - 16;
            t->context.rip = entry_point;
            t->context.rflags = 0x202; // IF (Interrupt Flag) установлен
            
            t->is_running = false;
            
            return t;
        }
    }
    return NULL; // Нет свободных слотов
}

// Вспомогательная функция для перехода в ring3
extern void task_enter_ring3(uint64_t rip, uint64_t rsp);

void task_start(struct task* task) {
    if (!task || task->is_running) {
        return;
    }
    
    task->is_running = true;
    current_task = task;
    
    // Переходим в ring3
    // Используем специальную функцию на ассемблере
    task_enter_ring3(task->context.rip, task->context.rsp);
}

