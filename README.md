# Moose Operating System

![Moose Logo](resources/MOOSE%20logo.png)
MOOSE is a 80s-style operating system, currently designed to run on virtual machines.

## Getting Started

As of writing, these instructions assume you are running MacOS with Homebrew installed. Feel free to adjust these commands so they can work on your Operating System. We are using Linux binutils, so Linux users can skip some of the prequisites.

### Prerequisites

Please install Homebrew, then install the following dependencies.

First, ap NativeOS' i386 elf toolchain and install it. This is used to link and compile the project.

```shell
brew tap nativeos/i386-elf-toolchain
brew install nativeos/i386-elf-toolchain/i386-elf-binutils
brew install nativeos/i386-elf-toolchain/i386-elf-gcc
```

Next, install QEMU and NASM. QEMU is used to test the operating system in a virtual environment, and NASM is used to compile ASM files.

Finally, install GRUB. We use GRUB to make the ISO.

```shell
brew install i686-elf-grub
```

### Installing

Since MOOSE has not yet been tested on a real device, here are the steps to install MOOSE on QEMU. Again, these instructions are for MacOS. Please adjust for your Operating System.

First, clone the repository.

```shell
git clone https://github.com/frogtheastronaut/moose-os.git
cd moose-os
```

Then, build the OS. This step is optional as there is most likely a pre-built binary in the bin/ directory. (MooseOS.iso)

```shell
make build-iso
```

Now you can run MooseOS with disk I/O support:

```shell
# Run with disk image (recommended)
make run

# Or run fullscreen with disk
make run-fullscreen
```

This will automatically create a 10MB disk image (`bin/moose_disk.img`) and run MooseOS with persistent storage support.

### Disk I/O Features

MooseOS now includes a full disk I/O subsystem with the following features:

- **ATA/IDE Controller Support**: Direct hardware interface for reading/writing sectors
- **Persistent Filesystem**: Files and directories are saved to disk and survive reboots
- **Disk Management Commands**: Use the terminal to manage the filesystem:
  - `diskinfo` - Show disk and filesystem information
  - `mount` - Mount the filesystem from disk
  - `sync` - Sync filesystem changes to disk
  - `save` - Save current filesystem state to disk
  - `load` - Load filesystem from disk
  - `format` - Format the disk with MooseOS filesystem

### Terminal Commands

Open the terminal in MooseOS and try these commands:

- `help` - Show available commands
- `ls` - List files and directories
- `cd <dir>` - Change directory
- `mkdir <name>` - Create directory
- `touch <name>` - Create file
- `cat <file>` - Show file content
- `diskinfo` - Show disk information
- `save` - Save filesystem to disk

Once it loads, select MooseOS, press Enter, and then MooseOS should load. You can move your cursor around, open the terminal, and test the persistent filesystem.

## Built With

- [Make](https://www.gnu.org/software/make/) - Runs Makefiles.
- [Homebrew](https://brew.sh/) - Install Dependencies

## Contributing

Please read [CONTRIBUTING.md](./CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning.

## Authors

- **Ethan Zhang** - *Initial work* - [frogtheastronaut](https://github.com/frogtheastronaut)

We will add contributors once we get contribution.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE) file for details
