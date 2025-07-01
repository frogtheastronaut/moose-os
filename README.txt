The Moose Operating System

GITHUB LINK HERE: https://github.com/frogtheastronaut/moose-os (all other files found there)
SOM LINK HERE: https://summer.hackclub.com/projects/555 (has Devlog and general project info)

Hi, welcome to the MOOSE OS project!
This project has a few features, and I am adding more.

This is a hobby project of mine. Hopefully it can be of use to you.

Enjoy!

Current Features:
- (Retro-themed) GUI
    VGA Graphics!
- Keyboard support
- File Explorer
    Press 'D' in the file explorer to make a folder, and press 'F' to make a file.
    Press 'Enter' to open a file/folder, and use arrow keys to change selection. Press Esc to exit.
- Text Editor
    The Text Editor auto saves your work, so just enter the file name in the dialog, and press
    Esc when you want to exit. 
- Terminal
    Terminal commands are: 
        ls (list directory), 
        cd (change directory), 
        mkdir (make directory), 
        touch (make file), 
        cat (read file), 
        clear (clear terminal), 
        time (get date and time) 
        and settimezone (change the time zone, eg. settimezone 10 to set timezone to UTC+10). 
    Press Esc to exit.
- Pong (game)
    W and S to move, you win after getting 11 points. Press Esc to exit, and Space to pause the game.
- Date and Time
    UTC time is shown by default. Use the terminal settimezone command to set your timezone.
    The time is always shown in the dock.

[How to run]
Please see the README.md located in the bin/ directory.

[How to build]
To build this code on a Mac, you can run this:
brew install i386-elf-binutils qemu nasm
and then you can run
make buildrun
to build and run MooseOS.




