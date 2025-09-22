[bits 32]

global gdt_flush
global tss_flush

; Flush the GDT 
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

; TSS flush
tss_flush:           
    mov   ax, [esp+4]
    ltr   ax
    ret