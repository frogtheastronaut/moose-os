# Makefile for SimpleM
# Copyright 2025 Ethan Zhang

# Makefile for building the code
buildrun:
	nasm -f elf32 src/kernel/kernel.asm -o kasm.o
	i386-elf-gcc -c src/kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T src/link.ld -o bin/simplem kasm.o kc.o 
	rm kasm.o kc.o
	qemu-system-i386 -kernel bin/simplem
run:
	qemu-system-i386 -kernel bin/simplem
build:
	nasm -f elf32 src/kernel/kernel.asm -o kasm.o
	i386-elf-gcc -c src/kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T src/link.ld -o bin/simplem kasm.o kc.o 
	rm kasm.o kc.o
clean:
	# In case the automatic cleaning breaks
	rm kasm.o kc.o
dbuild:
	# Delete build
	rm bin/simplem