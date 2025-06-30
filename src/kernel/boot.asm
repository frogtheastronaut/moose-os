; MooseOS
; Copyright 2025 Ethan Zhang, all rights reserved.

bits 32
; Multiboot spec
section .text
        align 4
        dd 0x1BADB002         
        dd 0x00               
        dd - (0x1BADB002 + 0x00) 

; Globals
global start
global start
global keyboard_handler
global read_port
global write_port
global load_idt
global timer_handler

; Externals, defined externally
extern kernel_main	       
extern keyboard_handler_main
extern task_tick

start:
  cli 			      
  mov esp, stack_space	
  call kernel_main      ; Kernel_main from kernel/kernel.c
  hlt		 	   

; IO PORTS - READ PORT
read_port:
	mov edx, [esp + 4]
	in al, dx	
	ret

; IO PORTS - WRITE PORT
write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret

; Interrupt Descriptor Table loader
load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret

; Keyboard Handler
keyboard_handler:                 
	call    keyboard_handler_main ; keyboard_handler_main from kernel/IDT.c
	iretd

; Timer interrupt handler for pre-emptive multitasking
timer_handler:
    pusha
    call task_tick
    popa
    ; Send EOI to PIC
    mov al, 0x20
    out 0x20, al
    iretd

; BSS
section .bss
resb 8192	
stack_space: