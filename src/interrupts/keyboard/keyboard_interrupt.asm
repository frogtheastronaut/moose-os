global keyboard_handler
extern keyboard_handler_main

keyboard_handler:                 
	call    keyboard_handler_main ; keyboard_handler_main from kernel/IDT.c
	iretd
 