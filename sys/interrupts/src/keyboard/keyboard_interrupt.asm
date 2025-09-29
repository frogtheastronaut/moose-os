global keyboard_handler
extern keyboard_handler_main

KERNEL_DATA_SEG equ 0x10

; handler for keyboard interrupts
keyboard_handler:
    ; save all registers
    pusha
    
    ; save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; set up kernel data segments
    mov ax, KERNEL_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; call the C handler
    call keyboard_handler_main
    
    ; send EoI to PIC
    mov al, 0x20
    out 0x20, al
    
    ; restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; restore all registers
    popa
    
    ; return from interrupt
    iretd
 