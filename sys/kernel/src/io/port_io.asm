global read_port
global write_port

; read port
read_port:
	mov edx, [esp + 4]
	in al, dx	
	ret

; write port
write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret