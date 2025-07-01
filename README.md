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
Please see the README.md located in the bin/ directory.

## How to build
To build this code on a Mac, you can run this:<br>
`brew install i386-elf-binutils qemu nasm`<br>
and then you can run
`make buildrun`
to build and run MooseOS.




