; MooseOS bootloader
; Copyright (c) 2025 Ethan Zhang
; Licensed under the MIT license. See license file for details

bits 32
; multiboot spec
section .text
        align 4
        dd 0x1BADB002         
        dd 0x00               
        dd - (0x1BADB002 + 0x00) 

; global
global start

; external variables
extern kernel_main	       

start:
  cli 			      
  mov esp, stack_space	
  call kernel_main      ; kernel_main from kernel/kernel.c
  hlt		 	   


; BSS
section .bss
resb 8192	
stack_space: