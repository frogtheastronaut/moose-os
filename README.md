# The Moose Operating System

## Welcome!

Hi, welcome to the MOOSE OS project!
This project has a few features, and I am adding more.

This is a hobby project of mine. Hopefully it can be of use to you.

Enjoy!

## Current Features 

- <strong>(Retro-themed) GUI</strong><br>
VGA Graphics!
- <strong>Keyboard support</strong><br>
- <strong>File Explorer</strong><br>
Press 'D' in the file explorer to make a folder, and press 'F' to make a file.
Press 'Enter' to open a file/folder, and use arrow keys to change selection. Press Esc to exit.
- <strong>Text Editor</strong><br>
The Text Editor auto saves your work, so just enter the file name in the dialog, and press
Esc when you want to exit. 
- <strong>Terminal</strong><br>
Terminal commands are:<br> `ls` (list directory),<br> `cd` (change directory),<br> `mkdir` (make directory),<br> `touch` (make file), <br>
`cat` (read file),<br> `clear` (clear terminal),<br> `time` (get date and time) and<br> `settimezone` (change the time zone, eg. `settimezone 10` to set timezone to UTC+10). Press Esc to exit.
- <strong>Pong (game)</strong><br>
W and S to move, you win after getting 11 points. Press Esc to exit, and Space to pause the game.
- <strong>Date and Time</strong><br>
UTC time is shown by default. Use the terminal `settimezone` command to set your timezone.
The time is always shown in the dock.

## How to run
Running this code is pretty straightforward (at least, on my Mac it is). Just install QEMU with your package manager. I use Brew, so I run <br>
`brew install qemu`<br>
And then, in the MooseOS directory, I run <br>
`make run`<br>
(run `make runfull` for fullscreen)<br>
And it should run bin/MooseOS.elf using `qemu-system-i386`<br>
So far, I have not experienced any issues, nor have recieved any feedback highlighting issues with running this code. If you have trouble running MooseOS, please start an issue in this repository, and I will do my best to help you.

https://github.com/user-attachments/assets/d57760de-c61f-4c09-913e-ae0f6f72ef77

## How to build
To build this code on a Mac, you can run this:<br>
`brew install i386-elf-binutils qemu nasm`<br>
and then you can run
`make buildrun`
to build and run MooseOS.




