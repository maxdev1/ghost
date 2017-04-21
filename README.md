# ABOUT GHOST
**Ghost** is an operating system for the Intel x86 platform. The project is licensed as GPLv3. It was started as a research project to learn more about low-level software programming and computer internals. This repository is occasionally updated with the new sources.

The kernel and the userspace applications are written from scratch in C++ and Assembly (and some C). The kernel is not based on any existing kernel and not Unix-compatible. There is a relatively small POSIX.1 compatibility layer that allows porting some software to the system though.

The most advanced features are SMP support (symmetric multiprocessing) to run on multiple processors, a v8086 monitor for executing BIOS calls (which also introduced VESA support), support for ELF binary loading (including thread-local-storage and all the other little things), as well as signal support. And there is a fancy little GUI that makes it all good-looking :-)

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

	max.schluessel@gmail.com
	
-Max Schlüssel
