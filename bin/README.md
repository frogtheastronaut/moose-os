## Running Moose OS
Running this code is pretty straightforward (at least, on my Mac it is). Just install QEMU with your package manager. I use Brew, so I run <br>
`brew install qemu`<br>
And then, in the MooseOS directory, I run <br>
`make run`<br>
(run `make runfull` for fullscreen)<br>
And it should run bin/MooseOS.elf using `qemu-system-i386`<br>
So far, I have not experienced any issues, nor have recieved any feedback highlighting issues with running this code. If you have trouble running MooseOS, please start an issue in this repository, and I will do my best to help you.

https://github.com/user-attachments/assets/d57760de-c61f-4c09-913e-ae0f6f72ef77