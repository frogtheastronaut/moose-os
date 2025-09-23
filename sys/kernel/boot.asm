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
global timer_handler
global page_fault_handler_asm

; external variables
extern kernel_main	       
extern timer_interrupt_handler
extern page_fault_handler_main

start:
  cli 			      
  mov esp, stack_space	
  call kernel_main      ; kernel_main from kernel/kernel.c
  hlt		 	   

; timer
KERNEL_DATA_SEG equ 0x10

timer_handler:
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Set up kernel data segments
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call the C handler
    call timer_interrupt_handler
    
    ; Send End of Interrupt to PIC
    mov al, 0x20
    out 0x20, al
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Return from interrupt
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