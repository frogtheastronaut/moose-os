# Color codes for terminal output
BLUE='\033[0;34m'
NC='\033[0m'
DPC_PREFIX="[${BLUE}DPC${NC}]"

echo -e "${DPC_PREFIX} Checking for MacOS recommended dependancies"
echo -e "${DPC_PREFIX} Checking for MacOS recommended dependancies"

gcc_installed=false
binutils_installed=false
nasm_installed=false
grub_installed=false
qemu_installed=false

# We want i386-elf-gcc
echo -e "${DPC_PREFIX} Checking for i386-elf-gcc..."
if ! command -v i386-elf-gcc &> /dev/null
then
	echo -e "${DPC_PREFIX} i386-elf-gcc could not be found."
else
	echo -e "${DPC_PREFIX} i386-elf-gcc is installed. Proceeding..."
	gcc_installed=true
fi

echo -e "${DPC_PREFIX} Checking for i386-elf-binutils..."
if ! command -v i386-elf-ld &> /dev/null
then
	echo -e "${DPC_PREFIX} i386-elf-binutils could not be found."
else
	echo -e "${DPC_PREFIX} i386-elf-binutils is installed. Proceeding..."
	binutils_installed=true
fi

echo -e "${DPC_PREFIX} Checking for nasm..."
if ! command -v nasm &> /dev/null
then
	echo -e "${DPC_PREFIX} NASM could not be found."
else
	echo -e "${DPC_PREFIX} NASM is installed. Proceeding..."
	nasm_installed=true
fi

echo -e "${DPC_PREFIX} Checking for GRUB.."
if ! command -v i686-elf-grub-mkrescue &> /dev/null
then
	echo -e "${DPC_PREFIX} GRUB could not be found."
else
	echo -e "${DPC_PREFIX} GRUB is installed. Proceeding..."
	grub_installed=true
fi

echo -e "${DPC_PREFIX} Checking for QEMU..."
if ! command -v qemu-system-i386 &> /dev/null
then
	echo -e "${DPC_PREFIX} QEMU could not be found."
else
	echo -e "${DPC_PREFIX} QEMU is installed. Proceeding..."
	qemu_installed=true
fi

if [ "$gcc_installed" = true ] && [ "$binutils_installed" = true ] && [ "$nasm_installed" = true ] && [ "$grub_installed" = true ] && [ "$qemu_installed" = true ]; then
	echo -e "${DPC_PREFIX} All recommended dependancies are installed. You are good to go!"
else
	echo -e "${DPC_PREFIX} Some recommended dependancies are missing. Please install the missing dependancies and try again."
	echo -e "${DPC_PREFIX} Missing dependancies:"
	if [ "$gcc_installed" = false ]; then
		echo "- i386-elf-gcc"
	fi
	if [ "$binutils_installed" = false ]; then
		echo "- i386-elf-binutils"
	fi
	if [ "$nasm_installed" = false ]; then
		echo "- NASM"
	fi
	if [ "$grub_installed" = false ]; then
		echo "- GRUB"
	fi
	if [ "$qemu_installed" = false ]; then
		echo "- QEMU"
	fi
fi
