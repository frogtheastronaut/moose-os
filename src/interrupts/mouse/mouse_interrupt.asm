global mouse_handler
extern mouse_handler_main

mouse_handler:
	pusha                   
	call    mouse_handler_main    ; mouse_handler_main from mouse.c
	popa                     
	iretd                       
