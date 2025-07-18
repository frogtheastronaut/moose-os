# Makefile for MooseOS
# Copyright 2025 Ethan Zhang, all rights reserved.

# Build and run targets
build-kernel:
	nasm -f elf32 src/kernel/boot.asm -o kasm.o
	nasm -f elf32 src/kernel/switchtask.asm -o switchtask.o
	i386-elf-gcc -c src/kernel/kernel.c -o kc.o -nostdlib -ffreestanding -O2 
	i386-elf-ld -m elf_i386 -T src/link.ld -o bin/MooseOS.elf kasm.o switchtask.o kc.o 
	rm kasm.o switchtask.o kc.o

run-kernel: build-kernel
	qemu-system-i386 -kernel bin/MooseOS.elf -m 128M

run-kernel-windowed: build-kernel
	qemu-system-i386 -kernel bin/MooseOS.elf -m 128M

run-kernel-fullscreen: build-kernel
	qemu-system-i386 -display cocoa,zoom-to-fit=on -kernel bin/MooseOS.elf -full-screen -m 128M

# ISO creation targets
build-iso: build-kernel
	# Create ISO directory structure
	mkdir -p iso/boot/grub
	# Copy kernel to ISO structure
	cp bin/MooseOS.elf iso/boot/
	# Create GRUB configuration
	echo 'menuentry "MooseOS" {' > iso/boot/grub/grub.cfg
	echo '    multiboot /boot/MooseOS.elf' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	# Create the ISO
	i686-elf-grub-mkrescue -o bin/MooseOS.iso iso
	# Clean up temporary directory
	rm -rf iso

run-iso: build-iso
	# Run the ISO in QEMU with 128MB RAM
	qemu-system-i386 -cdrom bin/MooseOS.iso -m 512M

run-iso-fullscreen: build-iso
	# Run the ISO in QEMU full screen with 128MB RAM
	qemu-system-i386 -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -full-screen -m 128M

# Cleanup targets
clean-iso:
	# Clean ISO files
	rm -f bin/MooseOS.iso
	rm -rf iso

clean-objects:
	# In case the automatic cleaning breaks
	rm kasm.o kc.o

clean-kernel:
	# Delete build
	rm bin/MooseOS.elf

clean-all: clean-iso clean-kernel clean-objects
	# Clean everything

all: build-kernel build-iso run-iso-fullscreen
	# build run EVERYTHING EVYERTHIG