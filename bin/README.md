## Running Moose OS
(I'm assuming you already have Make installed for running Makefiles.)
Running this code is rather easy. just install QEMU with your package manager.<br>
I'm using Homebrew, so I run: <br>
`brew install qemu`<br>
Then, I go to the MooseOS directory. For me its <br>
`cd ~/moose-os`<br>
Then I run:<br>
`make run-iso`<br>
and it should run `qemu-system-i386 -cdrom bin/MooseOS.iso -m 512M`<br>
If you wish a fullscreen experience, you can run: <br>
`make run-iso-fullscreen`<br>
and it should run `qemu-system-i386 -display cocoa,zoom-to-fit=on -cdrom bin/MooseOS.iso -full-screen -m 512M`<br>
If you have any trouble running MooseOS on fullscreen, please switch back to running it windowed. <br>
So far, I have not recieved any comments regarding issues with running the code. If it
does not work, please reach out to me either on this Github Repo, or through Summer of Making (for those reviewers).