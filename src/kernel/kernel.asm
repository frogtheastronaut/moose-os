;; Kernel assembly


bits 32
section .text
        ;multiboot
        align 4
        dd 0x1BADB002            ;magic
        dd 0x00                  ;flags
        dd - (0x1BADB002 + 0x00) ;checksum

global start
global start
global keyboard_handler
global read_port
global write_port
global load_idt
extern kernel_main	        ; kernel_main from kernel.c
extern keyboard_handler_main

start:
  cli 			;block interrupts
  mov esp, stack_space	;set stack pointer
  call kernel_main
  hlt		 	;halt the CPU

read_port:
	mov edx, [esp + 4]
	in al, dx	
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret
load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret
keyboard_handler:                 
	call    keyboard_handler_main
	iretd
section .bss
resb 8192		;8KB for stack
stack_space: