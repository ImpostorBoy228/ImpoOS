# Настройки компилятора и линковщика
CC = gcc
AS = nasm
LD = ld

# Флаги
CFLAGS = -ffreestanding -m64 -mno-red-zone -Isrc
LDFLAGS = -n -T linker.ld --no-warn-rwx-segments
ASFLAGS = -f elf64

# Список объектных файлов (генерируется из исходников в src/)
# Мы явно указываем порядок для загрузчика (header должен быть первым)
OBJ = src/header.o src/boot.o src/long_mode_init.o \
      src/kernel.o src/gdt.o src/idt.o src/terminal.o

# Главная цель
all: os.iso

# Сборка ISO
os.iso: kernel.bin grub.cfg
	@echo "--- Building ISO ---"
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o os.iso isodir
	@echo "--- Done ---"

# Линковка ядра
kernel.bin: $(OBJ)
	@echo "--- Linking Kernel ---"
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

# Правило для компиляции .c файлов
%.o: %.c
	@echo "--- Compiling C: $< ---"
	$(CC) -c $< -o $@ $(CFLAGS)

# Правило для компиляции .asm файлов
%.o: %.asm
	@echo "--- Assembling ASM: $< ---"
	$(AS) $(ASFLAGS) $< -o $@

# Очистка проекта
clean:
	rm -rf src/*.o kernel.bin os.iso isodir

# Запуск в QEMU
run: os.iso
	qemu-system-x86_64 -cdrom os.iso