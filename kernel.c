// kernel.c - freestanding 32-bit
void _start() {
    volatile unsigned short *video = (unsigned short*)0xB8000;
    
    // Очистка экрана: 80×25 = 2000 символов, белый на чёрном (0x0F00)
    for (int i = 0; i < 2000; i++) {
        video[i] = 0x0F00 | ' ';   // пробел с атрибутом белый на чёрном
    }
    
    // Теперь пишем "Hi"
    video[0] = 0x0F00 | 'H';
    video[1] = 0x0F00 | 'i';
    
    while(1) {}
}
