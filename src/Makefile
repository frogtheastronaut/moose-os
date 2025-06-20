# Makefile for SimpleM
# Copyright 2025 Ethan Zhang

# Simple Makefile for quickly running the code
run: 
	nasm -f elf32 kernel/boot.asm -o kasm.o
	i386-elf-gcc -c kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T link.ld -o os.elf kasm.o kc.o 
	qemu-system-i386 -kernel os.elf
	rm kasm.o kc.o os.elf
clean:
	rm kasm.o kc.o os
