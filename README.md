# ABOUT GHOST
**Ghost** is a hobby operating system for the Intel x86 platform.
The OS is written from scratch in C/C++ and Assembly.
The kernel is not based on any existing kernel, but is partially compatible to Unix.

This is the main development repository.

**Update 2020/10/08:** With the recent backmerge I've fixed a lot of bugs in the kernel and
rewrote a lot of the user-space code. Still work-in-progress, working towards a version 1.0.0.

*Project website:* http://ghostkernel.org/

## Documentation
See the `documentation` folder. It contains information about the technical design as well
as building instructions.

A usually up-to-date version of the documentation is provided here: https://ghostkernel.org/documentation/

## Features
* Currently x86-based
* Micro-kernel
* Multiboot-compliant
* Multi-processor (SMP) & multi-tasking support
* IPC; pipes, messages, shared memory
* libghostapi, kernel API library
* Home-made libc
* ELF binary & shared library support
* OS specific GCC toolchain
* C++ support
* Window server (GUI with home-made toolkit)
* VESA/VBE video driver
* PS/2 keyboard & mouse driver
* Serial COM1 kernel logging
* VM86 mode for BIOS calls

![Screenshot of 0.12.0](https://ghostkernel.org/files/ghost-0.12.0.png)

## Ported software
* musl (provides libm part of Ghost libc)
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
