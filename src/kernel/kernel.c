#include "include/tty.h"
#include "include/IDT.h"

void kernel_main(void) 
{
	/* Initialize terminal interface */
	terminal_initialize();
	terminal_writestring("Hello, kernel World!");
	terminal_writestring("Hello, kernel World!");
	idt_init();
	kb_init();
	while(1);
}