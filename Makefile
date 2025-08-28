
# Makefile for MooseOS
# Copyright 2025 Ethan Zhang, all rights reserved.

NASM = nasm
GCC = i386-elf-gcc
LD = i386-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386

build-elf:
	$(NASM) -f elf32 src/kernel/boot.asm -o kasm.o
	$(NASM) -f elf32 src/kernel/switchtask.asm -o switchtask.o
	$(GCC) -c src/kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	$(LD) -m elf_i386 -T src/link.ld -o bin/MooseOS.elf kasm.o switchtask.o kc.o 
	rm kasm.o switchtask.o kc.o

run-elf:
	$(QEMU) -kernel bin/MooseOS.elf

run-elf-fullscreen: 
	$(QEMU) -display cocoa,zoom-to-fit=on -kernel bin/MooseOS.elf -full-screen

# ISO creation targets
build-iso: build-elf
	mkdir -p iso/boot/grub
	cp bin/MooseOS.elf iso/boot/
	echo 'menuentry "MooseOS" {' > iso/boot/grub/grub.cfg
	echo '    multiboot /boot/MooseOS.elf' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o bin/MooseOS.iso iso
	rm -rf iso

run-iso:
	$(QEMU) -cdrom bin/MooseOS.iso -m 512M

run-iso-fullscreen:
	$(QEMU) -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -full-screen -m 512M

# Cleanup
clean-iso:
	rm -f bin/MooseOS.iso
	rm -rf iso

# clean objects if auto breaks
clean-o:
	rm kasm.o kc.o

clean-kernel:
	rm bin/MooseOS.elf

clean-all: clean-iso clean-kernel clean-o
	# Clean everything

all: build-elf build-iso run-iso-fullscreen
	# build run EVERYTHING EVYERTHIG