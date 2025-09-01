
NASM = nasm
GCC = i386-elf-gcc
LD = i386-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386


# Explicit list of all .c files in src/
SRC = \
    src/kernel/keyhandler.c \
    src/kernel/IDT.c \
    src/kernel/task.c \
    src/kernel/kernel.c \
    src/kernel/mouse.c \
    src/kernel/keyboard.c \
    src/kernel/disk.c \
    src/gui/explorer.c \
    src/gui/editor.c \
    src/gui/dock.c \
    src/gui/terminal.c \
    src/gui/gui.c \
    src/gui/fontdef.c \
    src/gui/images.c \
    src/time/rtc.c \
    src/filesys/file.c \
	src/kernel/lock.c

OBJ = $(SRC:.c=.o)

ASM_SRC := src/kernel/boot.asm src/kernel/switchtask.asm
ASM_OBJ := kasm.o switchtask.o

build-elf: $(ASM_OBJ) $(OBJ)
	$(LD) -m elf_i386 -T src/link.ld -o bin/MooseOS.elf $(ASM_OBJ) $(OBJ)

%.o: %.c
	$(GCC) -c $< -o $@ -nostdlib -ffreestanding -O2

kasm.o: src/kernel/boot.asm
	$(NASM) -f elf32 $< -o $@

switchtask.o: src/kernel/switchtask.asm
	$(NASM) -f elf32 $< -o $@

# Disk image targets
create-disk:
	@if [ ! -f bin/moose_disk.img ]; then \
		mkdir -p bin; \
		dd if=/dev/zero of=bin/moose_disk.img bs=1M count=10; \
		echo "Created 10MB disk image: bin/moose_disk.img"; \
	else \
		echo "Disk image bin/moose_disk.img already exists, skipping creation"; \
	fi

create-disk-force:
	@mkdir -p bin
	dd if=/dev/zero of=bin/moose_disk.img bs=1M count=10
	@echo "Created new 10MB disk image: bin/moose_disk.img (overwrote existing)"

# ISO creation targets
build-iso: build-elf
	mkdir -p iso/boot/grub
	cp bin/MooseOS.elf iso/boot/
	echo 'menuentry "MooseOS" {' > iso/boot/grub/grub.cfg
	echo '    multiboot /boot/MooseOS.elf' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o bin/MooseOS.iso iso
	rm -rf iso

# Run targets - ISO with disk is the primary way to run MooseOS
run: run-iso-with-disk

run-fullscreen: run-iso-with-disk-fullscreen

run-iso-with-disk: build-iso create-disk
	$(QEMU) -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -m 512M

run-iso-with-disk-fullscreen: build-iso create-disk
	$(QEMU) -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -full-screen -m 512M
	$(QEMU) -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -full-screen -m 512M

# Cleanup
clean-iso:
	rm -f bin/MooseOS.iso
	rm -rf iso

clean-disk:
	rm -f bin/moose_disk.img

clean-o:
	rm -f $(ASM_OBJ) $(OBJ)

clean-kernel:
	rm -f bin/MooseOS.elf

clean-all: clean-iso clean-disk clean-kernel clean-o
	@echo "Cleaned all build artifacts"

# Default target
all: build-iso
	@echo "MooseOS built successfully. Use 'make run' to start with disk support."

# Help target
help:
	@echo "MooseOS Build System"
	@echo "==================="
	@echo "Main targets:"
	@echo "  build-elf           - Build the kernel ELF file"
	@echo "  build-iso           - Build the ISO image"
	@echo "  create-disk         - Create a 10MB disk image (only if it doesn't exist)"
	@echo "  create-disk-force   - Create a new 10MB disk image (overwrites existing)"
	@echo "  run                 - Run MooseOS in QEMU with disk (default)"
	@echo "  run-fullscreen      - Run MooseOS in fullscreen with disk"
	@echo ""
	@echo "Cleanup targets:"
	@echo "  clean-o             - Remove object files"
	@echo "  clean-kernel        - Remove kernel ELF"
	@echo "  clean-iso           - Remove ISO files"
	@echo "  clean-disk          - Remove disk image"
	@echo "  clean-all           - Remove all build artifacts"
	@echo ""
	@echo "Note: The primary way to run MooseOS is with 'make run' which"
	@echo "      automatically builds the ISO and creates a disk image."