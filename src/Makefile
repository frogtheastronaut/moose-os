run: 
	nasm -f elf32 kernel/kernel.asm -o kasm.o
	i386-elf-gcc -c kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T link.ld -o os kasm.o kc.o 
	qemu-system-i386 -kernel os
	rm kasm.o kc.o os
build:
	nasm -f elf32 kernel/kernel.asm -o kasm.o
	i386-elf-gcc -c kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T link.ld -o ../bin/os kasm.o kc.o 
	rm kasm.o kc.o
brun:
	# build run
	qemu-system-i386 -kernel ../bin/os
qemu-run:
	qemu-system-i386 -kernel os
clean:
	rm kasm.o kc.o os

# # This is for testing the malloc
# malloc-ls:
# 	clang -O0 -g -W -Wall -Wextra -dynamiclib kernel/malloc/malloc.c -o malloc.dylib