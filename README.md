# ABOUT GHOST
**Ghost** is an operating system for the Intel x86 platform. The project is licensed as GPLv3.
The kernel and the userspace applications are written from scratch in C++ and Assembly (and some C).
The kernel is not based on any existing kernel and partially Unix-compatible.

This is the main development repository.

*Project website:* http://ghostkernel.org/

## Status of redux
With `redux` I started a re-implementation of the kernel which is now present in the `master` branch.
Currently I'm working on rewriting most of the filesystem related calls.
Once a ready-to-run version is there, I will post a live CD on the website.

If you are looking for the previous kernel implementation (versions up to 0.5.6b),
check the legacy branch out.

## Documentation
See the `documentation` folder. It contains information about the technical design as well
as building instructions.

## Features
* Pure micro-kernel
* Multiprocessor- & multitasking support (SMP)
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

![Screenshot of 0.5.6b](https://ghostkernel.org/files/ghost-0.5.6-highres.png)

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
