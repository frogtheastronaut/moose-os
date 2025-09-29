global mouse_handler
extern mouse_handler_main

KERNEL_DATA_SEG equ 0x10

; mouse handler
mouse_handler:
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
    call mouse_handler_main
    
    ; send EOI to both PICs (IRQ12 is on slave PIC)
    mov al, 0x20
    out 0xA0, al  ; send EOI to slave PIC
    out 0x20, al  ; send EOI to master PIC
    
    ; restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; restore all registers
    popa
    
    ; return from interrupt
    iretd                       
