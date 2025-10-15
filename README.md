<h1 align="center"><img src="./resources/moose-logo.png"/></h1>

![GitHub License](https://img.shields.io/github/license/frogtheastronaut/moose-os)  ![GitHub last commit (branch)](https://img.shields.io/github/last-commit/frogtheastronaut/moose-os/main) ![Build Status](https://github.com/frogtheastronaut/moose-os/actions/workflows/build.yml/badge.svg)

MOOSE is a 80s-style operating system. It can run on QEMU, Bochs, as well as real machines.

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
#### QEMU

First, clone the repository.

```shell
git clone https://github.com/frogtheastronaut/moose-os.git
cd moose-os
```

Then, just type

```shell
make
```

And it should install and run QEMU. It also tells you if it can't find any dependancies.

#### Real PC
This OS was tested on a 2009 HP PC equipped with VGA graphics and PS/2 input devices. While not all features have been exhaustively verified, a large majority function as intended.

BalenaEtcher was used to write the ISO to a USB, which boots and functions like a normal operating system.

## Features
Here is a list of features currently implemented
in MooseOS. There will be more in the future.

#### VGA Graphics
320x200 256-colour graphics mode

#### Desktop Interface
Dock-based application launcher

#### Keyboard and Mouse Support
Supports PS/2 keyboard and mouse interrupts, and also
displays mouse on screen

#### IDT
Full IDT with 32 exception handlers

#### Applications
Applications include: 
- terminal emulator (capable of some commands)
- a text editor
- file explorer.

#### Audio System
Hertz-based tone generation through the PC speaker.

#### ATA Disk I/O
Supports writing to a .img file through ATA read/write
operations.

#### Real-time clock
It syncs the time with the RTC at first, and then
uses the Programmable Interval timer to update the time.

#### ELF Program support
Can load, parse and validate ELF32 executables.

#### Memory Management
Custom heap allocator (`kmalloc`, `kfree`, etc) and 
4KBs of virtual memory with paging.

#### Custom filesystem
Custom filesystem with custom signature that you can
write to a .img file.

#### Kernel Panic system
Crash and reporting once an interrupt is triggered.

#### QEMU/Bochs Integration
Currently guaranteed to work on QEMU. Most, if not all
features work on Bochs though Bochs is pickier with errors. It also supports QEMU debug logging.

## Star History

<a href="https://www.star-history.com/#frogtheastronaut/moose-os&type=date&legend=top-left">
 <picture>
   <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=frogtheastronaut/moose-os&type=date&theme=dark&legend=top-left" />
   <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=frogtheastronaut/moose-os&type=date&legend=top-left" />
   <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=frogtheastronaut/moose-os&type=date&legend=top-left" />
 </picture>
</a>

## Built With

- [Make](https://www.gnu.org/software/make/) - Runs Makefiles.
- [Homebrew](https://brew.sh/) - Install Dependencies
- [GRUB](https://www.gnu.org/software/grub/) - Building ISO files.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details
