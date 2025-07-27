; Task Switching for Moose OS
; Copyright 2025 Ethan (go read other copryright files)
section .text
    global task_switch

task_switch:
    mov eax, [esp+4]      ; eax = &old_sp
    mov edx, esp          ; edx = current esp
    mov [eax], edx        ; *old_sp = esp

    mov eax, [esp+8]      ; eax = new_sp
    mov esp, eax          ; esp = new_sp

    pop edi
    pop esi
    pop ebx
    pop ebp

    ret
