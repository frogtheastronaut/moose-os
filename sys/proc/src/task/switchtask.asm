; MooseOS task switching assembly file
; Copyright (c) 2025 Ethan Zhang
; Licensed under the MIT license. See license file for details

section .text
global task_switch

task_switch:
    ; get parameters 
    mov eax, [esp+4]      ; eax = old stack pointer
    mov edx, [esp+8]      ; edx = new stack pointer
    
    ; save registers
    push ebp
    push ebx
    push esi
    push edi
    
    ; save current stack pointer to old task
    mov [eax], esp
    
    ; switch to new task's stack
    mov esp, edx
    
    ; restore new task's registers
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret
