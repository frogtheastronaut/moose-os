# This Makefile is assuming you have installed said dependancies
# We are also assuming other things. This includes:
# 1. You are using MacOS (anything but Arch)
# 2. You have the dependancies installed via Homebrew
# 3. You actually want to run the code.

# Dependancies are listed in README.md

RED = \033[0;31m
NC = \033[0m
MAKE_PREFIX = [$(RED)MAKE$(NC)]

NASM = nasm
GCC = i386-elf-gcc
INCLUDE_PATHS=$(shell find . -type d -name include)
NESTED_INCLUDE_PATHS=$(shell find . -type d -path '*/include/*')
ALL_INCLUDE_PATHS=$(INCLUDE_PATHS) $(NESTED_INCLUDE_PATHS)
INCLUDES=$(addprefix -I,$(ALL_INCLUDE_PATHS))
LD = i386-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386

SRC = $(shell find sys user -name "*.c" -type f)
OBJ = $(SRC:.c=.o)

ASM_SRC = $(shell find sys user -name "*.asm" -type f)
ASM_OBJ = $(ASM_SRC:.asm=.o)

all: preclean check_dependancies build-run clean-all

check_dependancies:
	@echo "$(MAKE_PREFIX) Running dependancy checker..."
	@# Check if MacOS. If not, user must manually install dependancies
	@bash -c 'if [ "$(shell uname)" = "Darwin" ]; then bash scripts/check_dependancy_mac.sh; else echo "User'\''s OS is not MacOS. Skipping dependancy checker."; fi'
	

build-elf: $(ASM_OBJ) $(OBJ)
	@echo "$(MAKE_PREFIX) Building ELF..."
	@$(LD) -m elf_i386 -T sys/link.ld -o bin/MooseOS.elf $(ASM_OBJ) $(OBJ)

%.o: %.c
	@$(GCC) -c $< -o $@ -nostdlib -ffreestanding -O2 $(INCLUDES)

%.o: %.asm
	@$(NASM) -f elf32 $< -o $@


create-disk:
	@if [ ! -f bin/moose_disk.img ]; then \
		mkdir -p bin; \
		dd if=/dev/zero of=bin/moose_disk.img bs=1M count=10; \
		echo "$(MAKE_PREFIX) Created 10MB disk image: bin/moose_disk.img"; \
	else \
		echo "$(MAKE_PREFIX) Disk image bin/moose_disk.img already exists, skipping creation"; \
	fi

build-iso: build-elf
	@echo "$(MAKE_PREFIX) Building ISO..."
	@mkdir -p iso/boot/grub
	@cp bin/MooseOS.elf iso/boot
	@# Copy files over from boot/grub/grub.cfg
	@cp sys/boot/grub/grub.cfg iso/boot/grub/
	@$(GRUB_MKRESCUE) -o bin/MooseOS.iso iso
	@rm -rf iso

build-run: build-iso create-disk run

run: create-disk
	@echo "$(MAKE_PREFIX) Running QEMU..."
	@$(QEMU) -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -m 512M -audiodev coreaudio,id=speaker -machine pcspk-audiodev=speaker -serial stdio

run-fullscreen:
	@$(QEMU) -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -full-screen -m 512M -serial stdio

# Note: There is no build-run-fullscreen target

rm-iso:
	@rm -f bin/MooseOS.iso
	@rm -rf iso

clean-disk:
	@rm -f bin/moose_disk.img

clean-o:
	@rm -f $(ASM_OBJ) $(OBJ)

clean-all:
	@echo "$(MAKE_PREFIX) Cleaning all object files.."
	@find . -name "*.o" -type f -delete
	@echo "$(MAKE_PREFIX) All .o files removed."

preclean:
	@echo "$(MAKE_PREFIX) Removing previous build files..."
	@find . -name "*.o" -type f -delete

clean-kernel:
	@rm -f bin/MooseOS.elf

run-bochs: create-disk
	@echo "$(MAKE_PREFIX) Running MooseOS with Bochs..."
	@echo "$(MAKE_PREFIX) Note: QEMU is reccommended for better performance."
	@bochs -f bin/bochsrc.txt -q 
	@echo "$(MAKE_PREFIX) Bochs session ended."
