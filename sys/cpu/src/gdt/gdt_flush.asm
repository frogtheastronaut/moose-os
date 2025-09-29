; MooseOS GDT and TSS flush system
; Copyright (c) 2025 Ethan Zhang and Contributors.
; Licensed under the MIT license. See license file for details.

[bits 32]

global gdt_flush
global tss_flush

; flush gdt
gdt_flush:
	mov eax, [esp+4] 
	lgdt [eax]      

	mov ax, 0x10  
	mov ds, ax     
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax
	jmp 0x08:.flush
.flush:
	ret

; flush tss
tss_flush:           
    mov   ax, [esp+4]
    ltr   ax
    ret