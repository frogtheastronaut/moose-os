======================== NOTE =========================================
This README document outlines a detailed list of features this OS has.
If you would like information on how to install it, check out README.md
The README.md should be in the same folder as this file.

Its GITHUB link is here:
https://github.com/frogtheastronaut/moose-os/blob/main/README.md

Thank you for your cooperation
=======================================================================

A DETAILED LIST OF FEATURES

1. VGA Graphics
	A grayscale VGA palette

	HOW TO TEST: Just look at the screen 

2. Keyboard and mouse support
	An IDT that supports keyboard and mouse interrupts

	HOW TO TEST: Move your mouse, left click, and type on your keyboard
	NOTE: Caps does not work. Use shift key instead

3. Custom stdlib
	Consists of many, but not all, standard C functions.
	Includes malloc!!!!

	HOW TO TEST: If other things work, this is definitely working. Every other thing relies on this

4. OS Paging

	HOW TO TEST: If it works, this works. Oh yeah, you can also type systest in the terminal

5. A GUI
	Retro-themed

	HOW TO TEST: Just look at it

6. A custom filesystem
	A custom MOOSE-OS filesystem, which supports writing to .img files through QEMU

	HOW TO TEST: Make a file, save it, and quit QEMU and boot it up again.

7. A file explorer, editor and dock
	Applications for MOOSE OS.

	HOW TO TEST:
		In dock, arrow keys to change apps, enter or click to open app.
		In file explorer, press d and f to make directories and files respectively, arrow keys to move and enter to change directory. ESC to quit.
		In editor, make a file name, press enter, spam, and ESC to quit.
		

8. Terminal 
	Can run some terminal commands, and also some custom debug ones.

	HOW TO TEST: In terminal, go in, type help for a list of cool commands, try some, and quit.

9. Custom bitmap images and font
	HOW TO TEST: Look at the screen.

10. Cooperate multitasking
	HOW TO TEST: If everything works, it's proof that multitasking is working.
	
11. Audio
	It plays rudimentary noises!

	HOW TO TEST: There should be a noise at startup. Also, there is a terminal beep command.
