[bits 32]

global idt_load

; load the idt
idt_load:
    push ebp
    mov ebp, esp

    mov eax, [esp + 8]
    lidt [eax]

    mov esp, ebp
    pop ebp
    ret
