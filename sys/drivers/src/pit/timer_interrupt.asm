global timer_handler
extern timer_interrupt_handler

; timer
KERNEL_DATA_SEG equ 0x10

timer_handler:
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
    
    ; call the C timer handler
    call timer_interrupt_handler

    ; send End of Interrupt to PIC
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