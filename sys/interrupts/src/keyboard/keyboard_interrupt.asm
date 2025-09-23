global keyboard_handler
extern keyboard_handler_main

KERNEL_DATA_SEG equ 0x10

keyboard_handler:
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
    call keyboard_handler_main
    
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
 