; Task Switching for Moose OS
; Copyright (c) 2025 Ethan (go read other copryright files)
section .text
    global task_switch

task_switch:
    ; Get parameters 
    mov eax, [esp+4]      ; eax = old stack pointer
    mov edx, [esp+8]      ; edx = new stack pointer
    
    ; Save registers
    push ebp
    push ebx
    push esi
    push edi
    
    ; Save current stack pointer to old task
    mov [eax], esp
    
    ; Switch to new task's stack
    mov esp, edx
    
    ; Restore new task's registers
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret
