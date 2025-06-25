## The Moose Operating System

# Welcome!
Hi, welcome to the MOOSE OS project!
This project has a few features, and I am adding more.

This is a hobby project of mine. Hopefully it can be of use to you.

Enjoy!

# Current Features
| Added/Working On    | To add?          |
| --------            | -------          |
| GUI                 | Mouse support    |
| File system         |                  |

# How to run
Running this code is pretty straightforward (at least, on my Mac). Just install QEMU with your package manager. I use Brew, so I run <br>
`brew install qemu`<br>
And then, in the MooseOS directory, I run <br>
`make run`<br>
And it should run bin/MooseOS.elf using `qemu-system-i386`


https://github.com/user-attachments/assets/d57760de-c61f-4c09-913e-ae0f6f72ef77


# How to build
To build this code on a Mac, you can run this:<br>
`brew install i386-elf-binutils qemu nasm`<br>
and then you can run
`make buildrun`
to build and run MooseOS.
(Disclaimer: Library installation methods may not be as simple as this. GCC and Make comes with my Mac, so I do not know how to install those. I assume your package manager can install them.)




