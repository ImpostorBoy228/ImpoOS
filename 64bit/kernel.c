#include <stdint.h>

void kernel_main() {
    // Указатель на видеопамять (цветной текст)
    // 0xB8000 - стандартный адрес для текста
    char* video_memory = (char*)0xb8000;

    const char* str = "ImpoOS (GRUB edition) - 64 bit is HERE!";
    
    // Очистка экрана (заполняем пробелами с черным фоном)
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        video_memory[i] = ' ';
        video_memory[i+1] = 0x07; // Серый на черном
    }

    // Пишем строку (Зеленый текст на черном)
    int j = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        video_memory[j] = str[i];
        video_memory[j+1] = 0x02; // 0x02 = Зеленый цвет
        j += 2;
    }

    while(1);
}