# ABOUT GHOST
**Ghost** is an operating system for the Intel x86 platform written
from scratch in C++ and assembly. It's based on a (almost) pure microkernel
design.

## Status
This is the bleeding edge source version of April 12, 2017.
Version 0.5.6

![Current highres-screenshot](https://ghostkernel.org/files/ghost-0.5.6-highres.png)

## Features
* Pure micro-kernel
* Multiprocessor- & multitasking support
* IPC - pipes, signals, messages, shared memory
* Window server (GUI with homemade toolkit)
* Patched GCC (OS specific toolchain)
* self-made libc
* libghostapi, extensive kernel API library
* libstdc++ port
* libghostuser for simplified file I/O, creating UIs & more...
* VESA video driver
* PS/2 keyboard & mouse driver
* ELF binary support
* Userspace filesystem driver support
* Serial COM1 kernel logging
* Virtual 8086 for BIOS calls
* Copy-on-write implementation, `fork()`

## Documentation
See the `documentation` folder for documentation. It contains information
about the technical design as well as building instructions.

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com
	
-Max Schlüssel
