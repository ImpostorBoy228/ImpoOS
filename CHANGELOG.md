# CHANGELOG

Подробное описание изменений и реализации компонентов ImpoOS.

## [2024] Реализация Ring3 TTY подсистемы

### Обзор

Реализована полноценная TTY (Teletypewriter) подсистема, позволяющая пользовательским программам в ring3 взаимодействовать с терминалом через системные вызовы, аналогично Linux tty.

---

## 1. Расширение GDT для поддержки Ring3

### Файлы: `src/gdt.c`

**Проблема:** Изначально GDT содержал только сегменты для ring0 (режим ядра). Для работы пользовательских программ в ring3 необходимы отдельные сегменты.

**Решение:**

Расширен массив GDT с 3 до 6 записей:

```c
// Структура GDT:
// 0: NULL дескриптор
// 1: Kernel Code (ring0, 64-bit)
// 2: Kernel Data (ring0)
// 3: User Code (ring3, 64-bit)  ← НОВОЕ
// 4: User Data (ring3)           ← НОВОЕ
// 5: TSS (зарезервировано)
```

**Технические детали:**

- **User Code сегмент:**
  - Access byte: `0xFA` = `11111010b`
    - Bit 7 (P): Present = 1
    - Bit 6-5 (DPL): Descriptor Privilege Level = 11 (ring3)
    - Bit 4 (S): System = 1 (code/data segment)
    - Bit 3 (E): Executable = 1
    - Bit 2 (DC): Direction/Conforming = 0
    - Bit 1 (RW): Readable = 1
    - Bit 0 (A): Accessed = 0
  - Granularity: `0x20` (Long Mode флаг)

- **User Data сегмент:**
  - Access byte: `0xF2` = `11110010b`
    - Аналогично User Code, но Executable = 0, Writable = 1

**Селекторы:**
- `0x08` = Kernel Code (1 * 8)
- `0x10` = Kernel Data (2 * 8)
- `0x18` = User Code (3 * 8)
- `0x20` = User Data (4 * 8)

---

## 2. TTY подсистема

### Файлы: `src/tty.c`, `src/tty.h`

**Цель:** Создать подсистему терминального ввода/вывода с буферизацией, аналогичную Linux tty.

### Архитектура TTY

```c
struct tty {
    // Буфер ввода (для чтения из ring3)
    char input_buffer[TTY_BUFFER_SIZE];  // 4096 байт
    size_t input_head;   // Позиция записи (producer)
    size_t input_tail;   // Позиция чтения (consumer)
    size_t input_count;  // Количество символов в буфере
    
    // Буфер вывода (для записи в ring3)
    char output_buffer[TTY_BUFFER_SIZE];
    size_t output_head;
    size_t output_tail;
    size_t output_count;
    
    // Флаги состояния
    bool echo;        // Эхо ввода (отображать нажатые клавиши)
    bool canonical;   // Канонический режим (обработка строк)
    
    // Текущая строка (для canonical режима)
    char line_buffer[TTY_MAX_LINE_LENGTH];  // 256 байт
    size_t line_pos;
};
```

### Реализованные функции

#### `tty_init()`
Инициализирует глобальный TTY, сбрасывает все буферы и флаги.

#### `tty_putchar_input(char c)`
**Вызывается из:** обработчика клавиатуры (ring0)

**Функциональность:**
- В **canonical режиме**:
  - Обрабатывает специальные символы:
    - `\n` / `\r` - завершение строки, копирование в input_buffer
    - `\b` / `127` (Backspace) - удаление последнего символа из line_buffer
    - Печатные символы (32-126) - добавление в line_buffer
  - Строка накапливается в `line_buffer` до нажатия Enter
  - При Enter вся строка копируется в `input_buffer` для чтения программой

- В **raw режиме** (canonical = false):
  - Символы сразу попадают в `input_buffer`

**Буферизация:**
- Используется кольцевой буфер (circular buffer)
- `input_head` - позиция записи (увеличивается при записи)
- `input_tail` - позиция чтения (увеличивается при чтении)
- `input_count` - количество доступных символов

#### `tty_read(char* buffer, size_t count)`
**Вызывается из:** системного вызова SYS_READ (ring3 → ring0)

**Функциональность:**
- Читает до `count` символов из `input_buffer`
- Использует кольцевой буфер для чтения
- Возвращает количество фактически прочитанных символов
- Блокирующий вызов (если буфер пуст, возвращает 0)

**Алгоритм:**
```c
while (read_count < count && t->input_count > 0) {
    buffer[read_count] = t->input_buffer[t->input_tail];
    t->input_tail = (t->input_tail + 1) % TTY_BUFFER_SIZE;
    t->input_count--;
    read_count++;
}
```

#### `tty_write(const char* buffer, size_t count)`
**Вызывается из:** системного вызова SYS_WRITE (ring3 → ring0)

**Функциональность:**
- Записывает данные напрямую на экран через `terminal_putchar()`
- В будущем можно добавить буферизацию вывода

**Интеграция:**
- Связана с низкоуровневым драйвером терминала (`terminal.c`)
- Каждый символ выводится немедленно на VGA экран

### Режимы работы TTY

#### Canonical режим (по умолчанию)
- Строки обрабатываются целиком
- Backspace работает локально (удаляет из line_buffer)
- Enter завершает строку и делает её доступной для чтения
- Аналогично терминалу Linux в cooked режиме

#### Raw режим (canonical = false)
- Каждый символ сразу попадает в input_buffer
- Нет обработки специальных символов
- Полезно для интерактивных приложений (редакторы, игры)

### Поток данных в TTY

```
Клавиатура (IRQ1)
    ↓
keyboard_handler_main() [ring0]
    ↓
tty_putchar_input() [ring0]
    ↓
input_buffer (кольцевой буфер)
    ↓
tty_read() [через syscall]
    ↓
Пользовательская программа [ring3]
```

---

## 3. Системные вызовы (Syscall/Sysret)

### Файлы: `src/syscall.c`, `src/syscall.h`, `src/syscall.asm`

**Цель:** Реализовать механизм перехода из ring3 в ring0 для выполнения привилегированных операций.

### Архитектура x86-64 SYSCALL

x86-64 предоставляет специальные инструкции для быстрого переключения между уровнями привилегий:

- **SYSCALL** - переход из ring3 в ring0
- **SYSRET** - возврат из ring0 в ring3

### Инициализация (`syscall_init()`)

#### Настройка MSR регистров

**IA32_STAR (0xC0000081):**
```c
uint64_t star = ((uint64_t)0x08 << 32) | ((uint64_t)0x18 << 48);
```
- Биты [47:32]: Селектор кода ядра (0x08 = Kernel Code)
- Биты [63:48]: Селектор кода пользователя (0x18 = User Code)

**IA32_LSTAR (0xC0000082):**
```c
uint64_t handler_addr = (uint64_t)syscall_handler_asm;
```
- Адрес обработчика системных вызовов (64-bit)

**IA32_FMASK (0xC0000084):**
```c
uint64_t fmask = (1 << 9) | (1 << 10); // IF и DF
```
- Маска флагов, которые очищаются при SYSCALL:
  - Bit 9 (IF): Interrupt Flag - прерывания отключаются
  - Bit 10 (DF): Direction Flag - направление строковых операций

**IA32_EFER (0xC0000080):**
```c
efer |= (1 << 0); // SCE (SYSCALL Enable)
```
- Включает поддержку инструкции SYSCALL

### Обработчик системных вызовов (`syscall_handler_asm`)

**Ассемблерная часть (`syscall.asm`):**

```asm
syscall_handler_asm:
    ; При SYSCALL процессор автоматически:
    ; - Сохраняет RIP в RCX
    ; - Сохраняет RFLAGS в R11
    ; - Переключается на ring0
    
    ; Сохраняем callee-saved регистры
    push rbx, rbp, r12-r15
    
    ; Сохраняем RCX и R11 (RIP и RFLAGS)
    push rcx
    push r11
    
    ; Подготавливаем аргументы для C функции
    ; RAX = номер системного вызова
    ; RDI = arg1
    ; RSI = arg2
    ; RDX = arg3
    mov rcx, rdx        ; arg3
    mov rdx, rsi        ; arg2
    mov rsi, rdi        ; arg1
    mov rdi, rax        ; syscall_num
    
    call syscall_handler
    
    ; Загружаем результат в RAX
    mov rax, [syscall_return_value]
    
    ; Восстанавливаем регистры
    pop r11, rcx
    pop r15-r12, rbp, rbx
    
    ; Возврат в ring3
    sysret  ; Использует RCX (RIP) и R11 (RFLAGS)
```

**C часть (`syscall_handler`):**

```c
void syscall_handler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    switch (syscall_num) {
        case SYS_READ:
            // arg1 = fd (игнорируется, используется TTY)
            // arg2 = buffer
            // arg3 = count
            char* buffer = (char*)arg2;
            size_t count = (size_t)arg3;
            size_t read = tty_read(buffer, count);
            syscall_return_value = read;
            break;
            
        case SYS_WRITE:
            // arg1 = fd
            // arg2 = buffer
            // arg3 = count
            const char* buffer = (const char*)arg2;
            size_t count = (size_t)arg3;
            size_t written = tty_write(buffer, count);
            syscall_return_value = written;
            break;
            
        case SYS_EXIT:
            // arg1 = exit_code
            // TODO: завершение процесса
            syscall_return_value = 0;
            break;
    }
}
```

### Использование системных вызовов из ring3

```c
static inline uint64_t syscall(uint64_t num, uint64_t arg1, uint64_t arg2, uint64_t arg3) {
    uint64_t ret;
    __asm__ volatile (
        "syscall"
        : "=a" (ret)                    // Результат в RAX
        : "a" (num),                    // Номер в RAX
          "D" (arg1),                   // arg1 в RDI
          "S" (arg2),                   // arg2 в RSI
          "d" (arg3)                    // arg3 в RDX
        : "rcx", "r11", "memory"       // Clobbered регистры
    );
    return ret;
}
```

**Calling convention для syscall:**
- RAX: номер системного вызова
- RDI: первый аргумент
- RSI: второй аргумент
- RDX: третий аргумент
- Возвращаемое значение: RAX

---

## 4. Модель задач для Ring3

### Файлы: `src/task.c`, `src/task.h`, `src/task.asm`

**Цель:** Создать базовую инфраструктуру для запуска пользовательских программ в ring3.

### Структура задачи

```c
struct task {
    uint64_t id;                    // Уникальный ID
    struct task_context context;     // Контекст выполнения
    uint64_t stack_base;            // Базовый адрес стека
    uint64_t stack_size;            // Размер стека (64 KB)
    bool is_running;                // Флаг выполнения
    uint64_t entry_point;           // Точка входа (адрес функции)
};

struct task_context {
    uint64_t rsp;      // Указатель стека
    uint64_t rip;      // Указатель инструкции
    uint64_t rflags;   // Флаги процессора
};
```

### Переход в Ring3 (`task_enter_ring3`)

**Ассемблерная реализация (`task.asm`):**

```asm
task_enter_ring3:
    ; rdi = RIP (точка входа)
    ; rsi = RSP (указатель стека)
    
    ; Подготавливаем стек для iretq
    ; Формат для iretq в 64-bit:
    ; [RSP+0x00] = RIP
    ; [RSP+0x08] = CS (code segment)
    ; [RSP+0x10] = RFLAGS
    ; [RSP+0x18] = RSP (stack pointer)
    ; [RSP+0x20] = SS (stack segment)
    
    push 0x23        ; SS (user data segment, ring3)
    push rsi         ; RSP (user stack)
    push 0x202       ; RFLAGS (IF установлен)
    push 0x1B        ; CS (user code segment, ring3)
    push rdi         ; RIP (точка входа)
    
    iretq            ; Переход в ring3
```

**Технические детали:**
- `0x1B` = User Code селектор (0x18) | RPL=3 = 0x1B
- `0x23` = User Data селектор (0x20) | RPL=3 = 0x23
- `0x202` = RFLAGS с установленным IF (Interrupt Flag)

### Создание задачи

```c
struct task* task_create(uint64_t entry_point, uint64_t stack_size) {
    // Находим свободный слот
    // Выделяем стек (статический массив)
    // Настраиваем контекст:
    //   - RSP = верхушка стека
    //   - RIP = entry_point
    //   - RFLAGS = 0x202 (IF установлен)
    return task;
}
```

### Запуск задачи

```c
void task_start(struct task* task) {
    task->is_running = true;
    current_task = task;
    task_enter_ring3(task->context.rip, task->context.rsp);
}
```

---

## 5. Интеграция компонентов

### Обновление обработчика клавиатуры

**Файл:** `src/keyboard.c`

**Изменения:**
```c
void keyboard_handler_main() {
    uint8_t scancode = inb(0x60);
    
    if (!(scancode & 0x80)) {
        char c = kbd_us[scancode];
        
        // Записываем в TTY буфер
        tty_putchar_input(c);
        
        // Если включен echo, выводим на экран
        struct tty* t = tty_get_current();
        if (t->echo) {
            terminal_putchar(c);
        }
    }
    
    outb(0x20, 0x20); // EOI
}
```

**Поток данных:**
1. Прерывание клавиатуры (IRQ1)
2. `keyboard_handler_main()` получает скан-код
3. Преобразование в символ
4. Запись в TTY буфер (`tty_putchar_input()`)
5. Эхо на экран (если включен)

### Инициализация в ядре

**Файл:** `src/kernel.c`

```c
void kernel_main() {
    gdt_install();      // GDT с ring3 сегментами
    idt_install();      // IDT для прерываний
    pic_remap();        // Настройка PIC
    
    tty_init();         // Инициализация TTY ← НОВОЕ
    syscall_init();     // Инициализация syscall ← НОВОЕ
    
    terminal_clear();
    kprint("ImpoOS 64-bit is loaded!\n");
    kprint("TTY subsystem initialized.\n");
    kprint("System calls enabled.\n");
    
    __asm__ volatile ("sti"); // Включаем прерывания
    
    while(1) {
        __asm__ volatile ("hlt");
    }
}
```

---

## 6. Пример пользовательской программы

### Файл: `src/user_program.c`

Демонстрирует использование системных вызовов из ring3:

```c
void user_main(void) {
    const char* msg = "Hello from ring3! This is a user program.\n";
    const char* prompt = "Enter something: ";
    char buffer[256];
    
    // Вывод через syscall
    syscall(SYS_WRITE, 1, (uint64_t)msg, strlen(msg));
    syscall(SYS_WRITE, 1, (uint64_t)prompt, strlen(prompt));
    
    // Чтение через syscall
    uint64_t read_count = syscall(SYS_READ, 0, (uint64_t)buffer, 255);
    
    // Эхо прочитанного
    const char* echo = "You typed: ";
    syscall(SYS_WRITE, 1, (uint64_t)echo, strlen(echo));
    syscall(SYS_WRITE, 1, (uint64_t)buffer, read_count);
    
    // Завершение
    syscall(SYS_EXIT, 0, 0, 0);
}
```

---

## 7. Обновление системы сборки

### Файл: `Makefile`

**Добавлены новые объектные файлы:**
```makefile
OBJ = ... \
      src/tty.o src/syscall.o src/syscall.asm.o \
      src/task.o src/task.asm.o
```

**Специальные правила для .asm файлов:**
```makefile
src/syscall.asm.o: src/syscall.asm
	$(AS) $(ASFLAGS) $< -o $@

src/task.asm.o: src/task.asm
	$(AS) $(ASFLAGS) $< -o $@
```

---

## Технические детали реализации

### Кольцевой буфер (Circular Buffer)

TTY использует кольцевой буфер для эффективного хранения данных:

```c
// Запись
buffer[head] = data;
head = (head + 1) % SIZE;
count++;

// Чтение
data = buffer[tail];
tail = (tail + 1) % SIZE;
count--;
```

**Преимущества:**
- O(1) операции вставки/удаления
- Фиксированный размер памяти
- Эффективное использование памяти

### Безопасность переходов между Ring3 и Ring0

**При SYSCALL:**
- Процессор автоматически переключается на ring0
- RIP и RFLAGS сохраняются в RCX и R11
- Прерывания отключаются (если настроено в FMASK)

**При SYSRET:**
- Процессор переключается обратно в ring3
- RIP восстанавливается из RCX
- RFLAGS восстанавливаются из R11
- Проверка привилегий выполняется автоматически

### Обработка прерываний в Ring3

Когда прерывание происходит в ring3:
1. Процессор автоматически переключается в ring0
2. Сохраняет контекст (RIP, RFLAGS, CS, SS, RSP)
3. Выполняет обработчик прерывания
4. Возвращается через IRETQ

---

## Будущие улучшения

### Планируемые функции:

1. **Множественные TTY**
   - Поддержка нескольких виртуальных терминалов
   - Переключение между TTY (Ctrl+Alt+F1-F6)

2. **Блокирующие системные вызовы**
   - Ожидание данных в `tty_read()` если буфер пуст
   - Планировщик задач для переключения контекста

3. **Управление процессами**
   - Полноценное создание/удаление процессов
   - Изоляция памяти между процессами
   - Сигналы и IPC

4. **Улучшенная обработка клавиатуры**
   - Поддержка специальных клавиш (Ctrl, Alt, Shift)
   - История команд
   - Автодополнение

5. **TTY режимы**
   - Raw режим для интерактивных приложений
   - Настройка через ioctl системный вызов

---

## Заключение

Реализована полноценная TTY подсистема с поддержкой ring3, позволяющая пользовательским программам взаимодействовать с терминалом через системные вызовы. Система архитектурно аналогична Linux tty и готова к дальнейшему расширению.

