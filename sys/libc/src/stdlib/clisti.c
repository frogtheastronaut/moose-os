#include "stdlib/clisti.h"

void cli(void) {
    __asm__ volatile ("cli");
}

void sti(void) {
    __asm__ volatile ("sti");
}
