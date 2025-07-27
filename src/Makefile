# Makefile for Moose OS
# Copyright 2025 Ethan Zhang

# Simple Makefile for quickly running the code
run: 
	nasm -f elf32 kernel/boot.asm -o kasm.o
	nasm -f elf32 kernel/switchtask.asm -o switchtask.o
	i386-elf-gcc -c kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 \
	    -I./kernel -I../gui/include
	i386-elf-ld -m elf_i386 -T link.ld -o os.elf kasm.o switchtask.o kc.o
	qemu-system-i386 -kernel os.elf
	rm kasm.o switchtask.o kc.o os.elf
run-full:
	nasm -f elf32 kernel/boot.asm -o kasm.o
	nasm -f elf32 kernel/switchtask.asm -o switchtask.o
	i386-elf-gcc -c kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 \
	    -I./kernel -I../gui/include
	i386-elf-ld -m elf_i386 -T link.ld -o os.elf kasm.o switchtask.o kc.o
	qemu-system-i386 -display cocoa,zoom-to-fit=on -kernel os.elf -full-screen
	rm kasm.o switchtask.o kc.o os.elf

clean:
	rm kasm.o switchtask.o kc.o os.elf
