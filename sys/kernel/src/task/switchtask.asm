; Task Switching for Moose OS
; Copyright (c) 2025 Ethan (go read other copryright files)
section .text
    global task_switch

task_switch:
    ; Get parameters BEFORE modifying stack
    mov eax, [esp+4]      ; eax = old_sp (pointer to old stack pointer)
    mov edx, [esp+8]      ; edx = new_sp (new stack pointer value)
    
    ; Save current task's registers
    push ebp
    push ebx
    push esi
    push edi
    
    ; Save current stack pointer to old task
    mov [eax], esp        ; *old_sp = current esp
    
    ; Switch to new task's stack
    mov esp, edx          ; esp = new_sp
    
    ; Restore new task's registers
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret
