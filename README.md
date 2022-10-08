# ABOUT GHOST
**Ghost** is a hobby operating system for the Intel x86 platform.
The entire OS is written from scratch in C/C++ and Assembly.
The kernel is not based on any existing kernel, but is partially compatible to Unix.

This is the main development repository.

**Update 2020/10/08:** With the recent backmerge I've fixed a lot of bugs in the kernel and
rewrote a lot of the user-space code. Still work-in-progress, working towards a version 1.0.0.

*Project website:* http://ghostkernel.org/

## Documentation
See the `documentation` folder. It contains information about the technical design as well
as building instructions.

## Features
* Pure micro-kernel
* Multiprocessor- & multitasking support (SMP)
* IPC - pipes, messages, shared memory
* GCC patches (OS specific toolchain)
* Self-made libc
* C++ support (libstdc++ port)
* libghostapi, extensive kernel API library
* ELF binary & shared library support
* Window server (GUI with homemade toolkit)
* VESA video driver
* PS/2 keyboard & mouse driver
* Userspace filesystem driver support
* Serial COM1 kernel logging
* Virtual 8086 for BIOS calls

![Screenshot of 0.10.0](https://ghostkernel.org/files/ghost-0.10.0.png)

## Ported software
* musl (libm part of Ghost's libc)
* cairo
* freetype
* libpng
* pixman
* zlib
* nasm
* bash (in progress)

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com
	
-Max Schlüssel
