# Assuming you have installed said dependancies
# Dependancies are listed in README.md
NASM = nasm
GCC = i386-elf-gcc
LD = i386-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386

# Make sure any additional file is listed in this SRC list.
SRC = \
    src/kernel/src/keyhandler.c \
    src/kernel/src/IDT.c \
    src/kernel/src/paging.c \
    src/kernel/src/task.c \
    src/kernel/src/kernel.c \
    src/kernel/src/mouse.c \
    src/kernel/src/keyboard.c \
    src/kernel/src/disk.c \
    src/kernel/src/vga.c \
    src/gui/src/explorer.c \
    src/gui/src/editor.c \
    src/gui/src/dock.c \
    src/gui/src/terminal.c \
    src/gui/src/gui.c \
    src/gui/src/fontdef.c \
    src/gui/src/images.c \
    src/time/src/rtc.c \
    src/filesys/src/file.c \
	src/kernel/src/lock.c \
	src/lib/src/malloc.c \
	src/lib/src/io.c \
	src/lib/src/stdio.c \
	src/lib/src/stdlib.c \
	src/lib/src/string.c

OBJ = $(SRC:.c=.o)

ASM_SRC := src/kernel/src/boot.asm src/kernel/src/switchtask.asm
ASM_OBJ := src/kasm.o src/switchtask.o

build-elf: $(ASM_OBJ) $(OBJ)
	$(LD) -m elf_i386 -T src/link.ld -o bin/MooseOS.elf $(ASM_OBJ) $(OBJ)

%.o: %.c
	$(GCC) -c $< -o $@ -nostdlib -ffreestanding -O2

src/kasm.o: src/kernel/src/boot.asm
	$(NASM) -f elf32 $< -o $@

src/switchtask.o: src/kernel/src/switchtask.asm
	$(NASM) -f elf32 $< -o $@


create-disk:
	@if [ ! -f bin/moose_disk.img ]; then \
		mkdir -p bin; \
		dd if=/dev/zero of=bin/moose_disk.img bs=1M count=10; \
		echo "Created 10MB disk image: bin/moose_disk.img"; \
	else \
		echo "Disk image bin/moose_disk.img already exists, skipping creation"; \
	fi

build-iso: build-elf
	mkdir -p iso/boot/grub
	cp bin/MooseOS.elf iso/boot/
	echo 'menuentry "MooseOS" {' > iso/boot/grub/grub.cfg
	echo '    multiboot /boot/MooseOS.elf' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o bin/MooseOS.iso iso
	rm -rf iso

build-run: build-iso create-disk
	$(QEMU) -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -m 512M

run: create-disk
	$(QEMU) -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -m 512M

run-fullscreen:
	$(QEMU) -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -drive file=bin/moose_disk.img,format=raw,if=ide -full-screen -m 512M

# Note: There is no build-run-fullscreen target

clean-iso:
	rm -f bin/MooseOS.iso
	rm -rf iso

clean-disk:
	rm -f bin/moose_disk.img

clean-o:
	rm -f $(ASM_OBJ) $(OBJ)

clean-kernel:
	rm -f bin/MooseOS.elf