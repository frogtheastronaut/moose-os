global mouse_handler
extern mouse_handler_main

KERNEL_DATA_SEG equ 0x10

mouse_handler:
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
    call mouse_handler_main
    
    ; Send End of Interrupt to both PICs (IRQ12 is on slave PIC)
    mov al, 0x20
    out 0xA0, al  ; Send EOI to slave PIC
    out 0x20, al  ; Send EOI to master PIC
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Return from interrupt
    iretd                       
