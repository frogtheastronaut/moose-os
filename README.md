<h1 align="center"><img src="./resources/moose-logo.png"/></h1>

![GitHub License](https://img.shields.io/github/license/frogtheastronaut/moose-os)  ![GitHub last commit (branch)](https://img.shields.io/github/last-commit/frogtheastronaut/moose-os/main) ![Build Status](https://github.com/frogtheastronaut/moose-os/actions/workflows/build.yml/badge.svg)

MOOSE is a 80s-style operating system, currently designed to run on virtual machines. \
A detailed list of features can be found in the [README.txt file](./README.txt)

## Getting Started

These instructions assume you are running MacOS with Homebrew installed. Feel free to adjust these commands so they can work on your Operating System. 
The Makefile will automatically adjust if you are running on Linux. Users on Linux
only need to install `nasm`, `xorriso` and `mtools`.

### Supported Virtual Machines
Most, if not all, virtual machines should work but we currently recommend *QEMU*. We
also support *Bochs*.

### Quick Start

Running this OS is relatively easy. Just install QEMU

```shell
brew install qemu
```
then run

```shell
cd moose-os
make run
```

And enjoy!

### Prerequisites

Please install Homebrew, then install the following dependencies.

First, tap NativeOS' i386 elf toolchain and install it. This is used to link and compile the project.

```shell
brew tap nativeos/i386-elf-toolchain
brew install nativeos/i386-elf-toolchain/i386-elf-binutils
brew install nativeos/i386-elf-toolchain/i386-elf-gcc
```

Next, install QEMU and NASM. QEMU is used to test the operating system in a virtual environment, and NASM is used to compile ASM files.

```shell
brew install qemu nasm
```

Finally, install GRUB. We use GRUB to make the ISO.

```shell
brew install i686-elf-grub
```

### Building

Since MOOSE has not yet been tested on a real device, here are the steps to install MOOSE on QEMU. Again, these instructions are for MacOS. Please adjust for your Operating System.

First, clone the repository.

```shell
git clone https://github.com/frogtheastronaut/moose-os.git
cd moose-os
```

Then, just type

```shell
make
```

And it should install. It also tells you if it can't find any dependancies.

## Built With

- [Make](https://www.gnu.org/software/make/) - Runs Makefiles.
- [Homebrew](https://brew.sh/) - Install Dependencies
- [GRUB](https://www.gnu.org/software/grub/) - Building ISO files.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details
