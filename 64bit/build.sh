#!/bin/bash
set -e

echo "=== Building ImpoOS with GRUB ==="

# 1. Компиляция ASM
nasm -f elf64 header.asm -o header.o
nasm -f elf64 boot.asm -o boot.o
nasm -f elf64 long_mode_init.asm -o long_mode_init.o

# 2. Компиляция C
gcc -c kernel.c -o kernel.o -ffreestanding -m64 -mno-red-zone

# 3. Линковка
ld -n -o kernel.bin -T linker.ld header.o boot.o long_mode_init.o kernel.o --no-warn-rwx-segments

# 4. Создание ISO структуры
mkdir -p isodir/boot/grub
cp kernel.bin isodir/boot/
cp grub.cfg isodir/boot/grub/

# 5. Сборка образа (GRUB сделает всё сам)
grub-mkrescue -o os.iso isodir

echo "=== Успех! Запускай: qemu-system-x86_64 -cdrom os.iso ==="