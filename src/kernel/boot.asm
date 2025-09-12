; MooseOS
; Copyright (c) 2025 Ethan Zhang.

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
global keyboard_handler
global mouse_handler
global read_port
global write_port
global load_idt
global timer_handler
global load_gdt
global page_fault_handler_asm

; external variables
extern kernel_main	       
extern keyboard_handler_main
extern mouse_handler_main
extern task_tick
extern page_fault_handler_main

start:
  cli 			      
  mov esp, stack_space	
  call kernel_main      ; kernel_main from kernel/kernel.c
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

; load IDT
load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti
	ret

; gdt flush function
load_gdt:
	mov eax, [esp+4] 
	lgdt [eax]      

	mov ax, 0x10  
	mov ds, ax     
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush
.flush:
	ret


keyboard_handler:                 
	call    keyboard_handler_main ; keyboard_handler_main from kernel/IDT.c
	iretd
 
mouse_handler:
	pusha                   
	call    mouse_handler_main    ; mouse_handler_main from kernel/mouse.c
	popa                     
	iretd                       

; timer
timer_handler:
    pusha
    call task_tick
    popa
    mov al, 0x20
    out 0x20, al
    iretd

; page fault handler
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