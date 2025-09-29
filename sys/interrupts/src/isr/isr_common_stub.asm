[bits 32]

KERNEL_DATA_SEG equ 0x10

extern isr_handler

global isr_common_stub

; ISR stub
isr_common_stub:
    pusha

    push ds
	push es
	push fs
	push gs

    xor eax, eax
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov eax, esp
    push eax

    call isr_handler
    add esp, 4

	pop gs
	pop fs
	pop es
	pop ds

    popa

    ; clean the error and the interrupt
    add esp, 8
    sti

    iret
