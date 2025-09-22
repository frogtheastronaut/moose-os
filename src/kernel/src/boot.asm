; MooseOS
; Copyright (c) 2025 Ethan Zhang and Contributors.

bits 32
; multiboot spec
section .text
        align 4
        dd 0x1BADB002         
        dd 0x00               
        dd - (0x1BADB002 + 0x00) 

; global
global start
global start
global mouse_handler
global read_port
global timer_handler
global page_fault_handler_asm

; external variables
extern kernel_main	       
extern mouse_handler_main
extern timer_interrupt_handler
extern page_fault_handler_main

start:
  cli 			      
  mov esp, stack_space	
  call kernel_main      ; kernel_main from kernel/kernel.c
  hlt		 	   

mouse_handler:
	pusha                   
	call    mouse_handler_main    ; mouse_handler_main from kernel/mouse.c
	popa                     
	iretd                       

; timer
timer_handler:
    pusha
    call timer_interrupt_handler
    popa
    iretd

; page fault handler (asm one)
page_fault_handler_asm:
    pusha
    mov eax, [esp + 32]  ; Get error code from stack
    push eax
    call page_fault_handler_main
    add esp, 4           ; Clean up stack
    popa
    add esp, 4           ; Remove error code from stack
    iretd

; BSS
section .bss
resb 8192	
stack_space: