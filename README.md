# ABOUT GHOST
**Ghost** is an operating system for the Intel x86 platform written
from scratch in C++ and assembly. It's based on a (almost) pure microkernel
design.

## Status
This is the bleeding edge source version of November 17, 2016.
Version 0.5.4

I've reworked a lot of stuff since the last revision and fixed many bugs.
The client-canvas is finished and used in the terminal/desktop. Started the
implementation of a new, VT100 compatible terminal and a custom shell
(`gosh`). Added various features to the window server, including event
transfer to the client, exit-event when the main window of a program closes,
remove components when their process dies and more.

Also added a JavaScript interpreter program (`js`) which bases on
the Duktape JS engine.

## Features
- Command-line & GUI environment
- Multiprocessor support
- PS/2 keyboard & mouse driver, VESA video driver
- Kernel written from scratch
- Patched GCC, OS specific toolchain
- Custom libc implementation
- libstdc++ port
- Extensive kernel API library (libapi)
- Userspace C++ library (libuser)
- Userspace filesystem driver support
- ELF binary support
- Various IPC mechanisms: pipes, signals, messages, shared memory
- Serial COM1 kernel logging
- Virtual 8086 for BIOS calls

## Documentation
See the `documentation` folder for documentation. It contains information
about the technical design as well as building instructions.

## Contact
If you want to get in contact, contribute to the project or have any questions,
feel free to contact me at:

	lokoxe@gmail.com
	
-Max Schlüssel
