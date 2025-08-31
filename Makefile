
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
    src/gui/explorer.c \
    src/gui/editor.c \
    src/gui/dock.c \
    src/gui/terminal.c \
    src/gui/gui.c \
    src/gui/fontdef.c \
    src/gui/images.c \
    src/time/rtc.c \
    src/filesys/file.c

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

clean-o:
	rm -f $(ASM_OBJ) $(OBJ)

clean-kernel:
	rm -f bin/MooseOS.elf

clean-all: clean-iso clean-kernel clean-o
	# Clean everything

all: build-elf build-iso run-iso-fullscreen
	# build run EVERYTHING EVYERTHIG