#ifndef SYSCALL_H
#define SYSCALL_H

#include <stdint.h>
#include <stddef.h>

// Номера системных вызовов
#define SYS_READ   0
#define SYS_WRITE  1
#define SYS_EXIT   2

// Инициализация системных вызовов
void syscall_init(void);

// Обработчик системного вызова (вызывается из ASM)
void syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3);

#endif // SYSCALL_H

