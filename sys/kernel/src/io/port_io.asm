global read_port
global write_port

; IO PORTS - READ PORT
read_port:
	mov edx, [esp + 4]
	in al, dx	
	ret

; IO PORTS - WRITE PORT
write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret