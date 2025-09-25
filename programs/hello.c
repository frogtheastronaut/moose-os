#define VGA_ADDRESS 0xB8000
#define WHITE_ON_BLACK 0x0F

void print(const char* str) {
    volatile char* vga = (volatile char*)VGA_ADDRESS;
    while (*str) {
        *vga++ = *str++;
        *vga++ = WHITE_ON_BLACK;
    }
}

void _start() {
    print("Hello, world!");
    for (;;);
}