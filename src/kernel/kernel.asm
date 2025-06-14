;; Kernel assembly


bits 32
section .text
        ;multiboot
        align 4
        dd 0x1BADB002            ;magic
        dd 0x00                  ;flags
        dd - (0x1BADB002 + 0x00) ;checksum

global start
extern kernel_main	        ; kernel_main from kernel.c

start:
  cli 			;block interrupts
  mov esp, stack_space	;set stack pointer
  call kernel_main
  hlt		 	;halt the CPU


section .bss
resb 8192		;8KB for stack
stack_space: